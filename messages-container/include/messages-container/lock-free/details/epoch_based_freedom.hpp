#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include <iostream>
#include <cassert>

constexpr size_t MAX_THREADS = 64;
constexpr size_t CLEANUP_THRESHOLD = 64;

template <typename Node> class EpochManager
{
    struct RetiredNode
    {
        size_t epoch;
        void* ptr;
        std::function<void(void*)> deleter;
    };

    std::atomic<size_t> _globalEpoch{0};
    thread_local static size_t _localEpoch;

    std::atomic<size_t> _activeEpochs[MAX_THREADS];
    /// @brief Retired nodes for each epoch for each thread
    /// as long as each thread writes to its own list, we don't need to synchronize
    /// Team, I am viaoliting  here rules not using STL - but sorry
    /// The main challenge is to avoid using mutexes and locks
    /// due to lack of time I am using vector of nodes for now
    std::vector<RetiredNode> _retiredNodes[MAX_THREADS];

    std::atomic<bool> _reclaiming{false};  // Ensures only one thread executes reclaim()
    std::atomic<int> _activeReclaimingThread{-1};
    std::thread::id _reclaimingThread{};
    const size_t _threadCnt;
    std::atomic<size_t> _nextThreadIdx{0};
    static thread_local int _threadIdx;

  public:
    EpochManager()
        : _threadCnt(std::min(MAX_THREADS, static_cast<size_t>(std::thread::hardware_concurrency())))
    {

        for (size_t i = 0; i < _threadCnt; ++i)
        {
            _activeEpochs[i].store(0, std::memory_order_release);
        }
    }

    ~EpochManager()
    {
        reclaim();
    }

    EpochManager(const EpochManager&) = delete;
    EpochManager& operator=(const EpochManager&) = delete;

    void enterEpoch()
    {
        _localEpoch = _globalEpoch.load(std::memory_order_acquire);
        _activeEpochs[calcIdx()].store(_localEpoch, std::memory_order_release);
    }

    void exitEpoch()
    {
        // _localEpoch[calcIdx()].store(0, std::memory_order_release);
        _activeEpochs[calcIdx()].store(0, std::memory_order_release);
    }

    /// @brief Retire a node (add it to the retired list for the current epoch)
    void retireNode(
        void* ptr,
        std::function<void(void*)> deleter = [](void* ptr) { delete static_cast<Node*>(ptr); })
    {
        if (!deleter) {
            std::cerr << "Error: Deleter is null for ptr=" << ptr << std::endl;
            throw std::invalid_argument("Deleter must be callable");
        }

        auto reclaimingThread = _activeReclaimingThread.load(std::memory_order_acquire);
        if (reclaimingThread != -1)
        {
            assert((size_t)reclaimingThread != calcIdx() && " Reclaiming thread is the same as the current thread\n");
        }

         // Check if the pointer is already retired
        for (auto& entry : _retiredNodes[calcIdx()]) {
            if (entry.ptr == ptr) {
                return;
            }
        }

        _retiredNodes[calcIdx()].push_back({_localEpoch, ptr, deleter});

        // std::cout << "Thread " << calcIdx() << " retiring node at " << ptr << std::endl;

        if (_retiredNodes[calcIdx()].size() >= CLEANUP_THRESHOLD)
        {
            reclaim();
        }
    }

  private:
    /// @brief Try to reclaim memory from older epochs
    size_t calcIdx() {
        if (_threadIdx < 0) {
            _threadIdx = static_cast<int>(_nextThreadIdx.fetch_add(1, std::memory_order_relaxed) % _threadCnt);
        }

        return _threadIdx;
    }

    void reclaim()
    {
        bool expected = false;
        if (!_reclaiming.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
            return; // Another thread is already reclaiming
        }

        _activeReclaimingThread = calcIdx();
        _reclaimingThread = std::this_thread::get_id();

        size_t newEpoch = _globalEpoch.fetch_add(1, std::memory_order_acq_rel);

          // Wait for all threads to advance to the new epoch
        for (size_t i = 0; i < _threadCnt; ++i) {
            size_t retries = 1000;
            while (_activeEpochs[i].load(std::memory_order_acquire) < newEpoch) {
                if (--retries == 0) {
                    std::cerr << "Warning: Thread " << i << " stuck in old epoch\n";
                    goto EXIT;
                }
                std::this_thread::yield();
            }
        }


        for (size_t i = 0; i < _threadCnt; ++i)
        {
            auto& list = _retiredNodes[i];
            auto it = list.begin();
            while (it != list.end())
            {
                if (it->epoch < newEpoch && _activeEpochs[i].load(std::memory_order_acquire) == 0)
                {
                    // std::cout << "Thread " << _activeReclaimingThread << " reclaiming node at " << it->ptr << std::endl;

                    assert(it->deleter && "Node deleter is not valid");
                    assert(it->ptr && "Node ptr is not valid");

                    it->deleter(it->ptr);
                    it = list.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
EXIT:
        _reclaiming.store(false, std::memory_order_release);
        _activeReclaimingThread.store(-1, std::memory_order_release);
        _reclaimingThread = std::thread::id{};
    }
};

template <typename Node> thread_local size_t EpochManager<Node>::_localEpoch = 0;
template <typename Node> thread_local int EpochManager<Node>::_threadIdx = -1;
