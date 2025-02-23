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
    const size_t _threadCnt;
    std::atomic<size_t> _nextThreadIdx{0};
    static thread_local size_t _threadIdx;

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
        for (auto& retired : _retiredNodes)
        {
            for (auto node : retired)
            {
                node.deleter(node.ptr);
            }
        }
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
            assert((size_t)reclaimingThread != calcIdx() && "Reclaiming thread is the same as the current thread");
        }

         // Check if the pointer is already retired
        for (auto& entry : _retiredNodes[calcIdx()]) {
            if (entry.ptr == ptr) {
                std::cerr << "Error: Pointer " << ptr << " already retired" << std::endl;
                return;
            }
        }

        _retiredNodes[calcIdx()].push_back({_localEpoch, ptr, deleter});

        std::cout << "Retired ptr=" << ptr << " in epoch=" << _globalEpoch.load() << std::endl;

        if (_retiredNodes[calcIdx()].size() >= CLEANUP_THRESHOLD)
        {
            reclaim();
        }
    }

  private:
    /// @brief Try to reclaim memory from older epochs
    size_t calcIdx() {
        if (_threadIdx == std::numeric_limits<size_t>::max()) {
            _threadIdx = _nextThreadIdx.fetch_add(1, std::memory_order_relaxed) % _threadCnt;
        }
        return _threadIdx;
    }
    /*
    size_t calcIdx() const
    {
        return std::hash<std::thread::id>{}(std::this_thread::get_id()) % _threadCnt;
    }
        */

    void reclaim()
    {
        bool expected = false;
        if (!_reclaiming.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
            return; // Another thread is already reclaiming
        }

        _activeReclaimingThread = calcIdx();

        size_t newEpoch = _globalEpoch.fetch_add(1, std::memory_order_acq_rel);

          // Wait for all threads to advance to the new epoch
        for (size_t i = 0; i < _threadCnt; ++i) {
            while (_activeEpochs[i].load(std::memory_order_acquire) < newEpoch) {
                std::this_thread::yield();
            }
        }


        for (size_t i = 0; i < _threadCnt; ++i)
        {
            auto& list = _retiredNodes[i];
            auto it = list.begin();
            while (it != list.end())
            {
                if (it->epoch < newEpoch)
                {
                    std::cout << "Reclaiming ptr=" << it->ptr << std::endl;

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

        _reclaiming.store(false, std::memory_order_release);
        _activeReclaimingThread.store(-1, std::memory_order_release);
    }
};

template <typename Node> thread_local size_t EpochManager<Node>::_localEpoch = 0;
template <typename Node> thread_local size_t EpochManager<Node>::_threadIdx = std::numeric_limits<size_t>::max();
