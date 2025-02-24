#pragma once

#include <message.hpp>

#include "shared_mutex.hpp"
#include "spin_lock.hpp"

#include <atomic>
#include <cstddef>
#include <cstring>
#include <functional>
#include <future>
#include <iostream>

/// @brief  A hash map that uses chaining to resolve collisions with spin locks
/// @tparam Size
template <size_t Size = 1024> class HashMap
{
    struct HashEntry
    {
        Message message{};
        HashEntry* next{nullptr};
    };

    std::future<void> _rehashFuture;  // Background thread for rehashing
    std::atomic<bool> _stopRehashing{false};
    std::atomic<size_t> _capacity{Size};
    std::atomic<size_t> _size{0};

    std::unique_ptr<HashEntry*[]> _table{nullptr};
    // std::shared_ptr<Spinlock[]> _locks{nullptr};
    std::shared_ptr<SharedMutex[]> _locks{nullptr};
    mutable std::shared_mutex _globalMutex;

    static constexpr float LOAD_FACTOR = 0.75f;

  private:
    size_t hash(uint64_t key, size_t capacity) const
    {
        return std::hash<uint64_t>{}(key) & (capacity - 1);
    }

    void rehash()
    {
        std::cout << "resing !!!!!!!!!!!!!!!!" << std::endl;
        std::unique_lock<std::shared_mutex> globalLock(_globalMutex);

        auto capacity = _capacity.load(std::memory_order_acquire);
        size_t newCapacity = capacity << 1;
        std::unique_ptr<HashEntry*[]> newTable(new HashEntry*[newCapacity]);

        for (size_t i = 0; i < capacity; ++i)
        {
            HashEntry* entry = _table[i];
            while (entry)
            {
                HashEntry* next = entry->next;
                size_t index = hash(entry->message.MessageId, newCapacity);

                entry->next = newTable[index];
                newTable[index] = entry;

                entry = next;
            }
        }

        std::unique_ptr<HashEntry*[]> oldTable = std::move(_table);
        std::shared_ptr<SharedMutex[]> newLockPool = std::make_shared<SharedMutex[]>(newCapacity);

        for (size_t i = 0; i < newCapacity; ++i)
        {
            newLockPool[i] = SharedMutex();  // Create new mutexes
        }

        _table = std::move(newTable);
        _locks = newLockPool;
        _capacity.store(newCapacity, std::memory_order_release);
    }

  public:
    HashMap()
        : _table(std::make_unique<HashEntry*[]>(Size))
        , _locks(std::make_shared<SharedMutex[]>(Size))
    // , _locks(std::make_shared<Spinlock[]>(Size))
    {
        _rehashFuture = std::async(std::launch::async, &HashMap::rehashMonitor, this);
    }

    ~HashMap()
    {
        _stopRehashing.store(true, std::memory_order_relaxed);
        if (_rehashFuture.valid())
        {
            _rehashFuture.get();  // Wait for the rehashing thread to finish
        }

        auto capacity = _capacity.load(std::memory_order_relaxed);

        // Delete all entries in the hash table
        for (size_t i = 0; i < capacity; ++i)
        {
            HashEntry* entry = _table[i];
            while (entry)
            {
                HashEntry* next = entry->next;
                delete entry;
                entry = next;
            }
        }
    }

    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;
    HashMap(HashMap&&) = delete;
    HashMap& operator=(HashMap&&) = delete;

    void rehashMonitor()
    {
        while (!_stopRehashing.load(std::memory_order_relaxed))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (_size.load(std::memory_order_acquire) >= _capacity.load(std::memory_order_acquire) * LOAD_FACTOR)
            {
                rehash();
            }
        }
    }

    bool insert(const Message& message)
    {
        std::shared_lock<std::shared_mutex> globalLock(_globalMutex);
        // std::shared_ptr<Spinlock[]> lockPool = _locks;

        std::shared_ptr<SharedMutex[]> lockPool = _locks;
        size_t index = hash(message.MessageId, _capacity.load(std::memory_order_relaxed));

        lockPool[index].lock();

        HashEntry* entry = _table[index];
        HashEntry* prev = nullptr;

        while (entry)
        {
            if (entry->message.MessageId == message.MessageId)
            {
                lockPool[index].unlock();
                return false;
            }

            prev = entry;
            entry = entry->next;
        }

        // Insert new message
        HashEntry* newEntry = new HashEntry{message, nullptr};
        if (prev)
        {
            prev->next = newEntry;
        }
        else
        {
            _table[index] = newEntry;
        }

        _size.fetch_add(1, std::memory_order_release);

        lockPool[index].unlock();
        return true;
    }

    bool find(uint64_t messageId, Message& result) const
    {
        std::shared_lock<std::shared_mutex> globalLock(_globalMutex);
        // std::shared_ptr<Spinlock[]> lockPool = _locks;
        std::shared_ptr<SharedMutex[]> lockPool = _locks;
        size_t index = hash(messageId, _capacity.load(std::memory_order_relaxed));

        lockPool[index].lock();

        HashEntry* entry = _table[index];
        while (entry)
        {
            if (entry->message.MessageId == messageId)
            {
                result = entry->message;
                lockPool[index].unlock();
                return true;
            }

            entry = entry->next;
        }

        lockPool[index].unlock();
        return false;
    }

    bool remove(uint64_t messageId)
    {
        std::shared_lock<std::shared_mutex> globalLock(_globalMutex);
        std::shared_ptr<SharedMutex[]> lockPool = _locks;
        size_t index = hash(messageId, _capacity.load(std::memory_order_relaxed));
        // std::shared_ptr<Spinlock[]> lockPool = _locks;

        lockPool[index].lock();

        HashEntry* entry = _table[index];
        HashEntry* prev = nullptr;

        while (entry)
        {
            if (entry->message.MessageId == messageId)
            {
                if (prev)
                {
                    prev->next = entry->next;
                }
                else
                {
                    _table[index] = entry->next;
                }

                delete entry;
                _size.fetch_sub(1, std::memory_order_release);

                lockPool[index].unlock();
                return true;
            }

            prev = entry;
            entry = entry->next;
        }

        lockPool[index].unlock();
        return false;
    }

    size_t size() const
    {
        return _size.load(std::memory_order_acquire);
    }

    size_t capacity() const
    {
        return _capacity.load(std::memory_order_acquire);
    }

    void debug()
    {
        std::shared_lock<std::shared_mutex> globalLock(_globalMutex);
        for (size_t i = 0; i < _capacity.load(std::memory_order_relaxed); ++i)
        {
            _locks[i].lock();

            HashEntry* entry = _table[i];
            while (entry)
            {
                std::cout << "Index: " << i << " MessageId: " << entry->message.MessageId << std::endl;
                entry = entry->next;
            }

            _locks[i].unlock();
        }
    }
};
