#pragma once

#include "details/epoch_based_freedom.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <thread>

/// @brief: Lock-free hashmap with dynamic resizing and linear probing
/// The map is implemented as an array of buckets, each bucket is a linked list of nodes
/// Each node contains a message and a pointer to the next node
/// The map is lock-free, but not wait-free, because resizing is done with CAS
/// Could be done as template class -> template <size_t BUCKET_COUNT = 8191>

constexpr uintptr_t DELETED_MARK = 0b01;

template <typename Value, size_t Size = 8191> class LF_HashMap
{
    struct Node
    {
        Value message;
        std::atomic<Node*> next;

        Node(const Value& msg, Node* next_ = nullptr)
            : message(msg)
            , next(next_)
        {
        }
    };

    struct Bucket
    {
        std::atomic<Node*> head{nullptr};
    };

    ///@NOTE: atomic<shared_ptr<Node>> would be better, but C++20 standard impl is not lock free
    /// there are other not commonly used implementation, but they are very platform specific
    /// so let's stick with atomic<Node*> for now, raw pointers, is not  perfect, but it's lock free

    std::atomic<Bucket*> _table{nullptr};
    std::atomic<size_t> _size{0};  // Current number of elements
    std::atomic<size_t> _capacity;
    std::atomic<bool> _resizing{false};

    static constexpr float LOAD_FACTOR = 0.75f;

    std::unique_ptr<EpochManager<Node>> _epochManager;

  private:  // Private methods
    size_t hash(uint64_t messageId, size_t bucket_count) const
    {
        return std::hash<uint64_t>{}(messageId) % bucket_count;
    }

    Bucket* createBucketArray(size_t newSize);

    void tryResize()
    {
        bool expected = false;
        if (!_resizing.compare_exchange_strong(expected, true, std::memory_order_acquire))
        {
            return; // Another thread is already resizing
        }

        resize();

        _resizing.store(false, std::memory_order_release);
    }

    void resize();
    void clearInternal();

  public:
    void debug() const
    {
        for (size_t i = 0; i < _capacity; ++i)
        {
            Node* node = _table[i].head.load();
            while (node)
            {
                printf("Bucket %zu: ", i);
                printf("%lu \n", node->message.MessageId);
                node = node->next.load();
            }
        }
    }

    LF_HashMap();
    ~LF_HashMap();

    LF_HashMap(const HashMap&) = delete;
    LF_HashMap(HashMap&&) = delete;
    LF_HashMap& operator=(const HashMap&) = delete;
    LF_HashMap&& operator=(HashMap&&) = delete;

    // Insert a message (drop duplicates)
    bool insert(const Value& msg);

    // Find a message by MessageId
    bool find(uint64_t messageId, Value& result);

    // Remove a message by MessageId
    bool remove(uint64_t messageId);

    // Get the current number of elements
    size_t size() const
    {
        return _size.load(std::memory_order_acquire);
    }

};

template <typename Value, size_t Size>
LF_HashMap<Value, Size>::LF_HashMap()
    : _capacity(Size)
    , _epochManager(std::make_unique<EpochManager<Node>>())
{
    _table.store(createBucketArray(Size), std::memory_order_release);
}

template <typename Value, size_t Size> LF_HashMap<Value, Size>::~HashMap()
{
    clearInternal();
}

template <typename Value, size_t Size> void LF_HashMap<Value, Size>::clearInternal()
{
    auto table_p = _table.load();

    for (size_t i = 0; i < _capacity.load(); ++i)
    {
        Node* curr = table_p[i].head.load();
        while (curr)
        {
            Node* next = curr->next.load();
            delete curr;
            curr = next;
        }
    }

    _size.store(0);

    delete[] table_p;
}

template <typename Value, size_t Size>
typename LF_HashMap<Value, Size>::Bucket* LF_HashMap<Value, Size>::createBucketArray(size_t newSize)
{
    Bucket* newBuckets = new Bucket[newSize];

    for (size_t i = 0; i < newSize; ++i)
    {
        newBuckets[i].head.store(nullptr, std::memory_order_relaxed);
    }

    return newBuckets;
}

template <typename Value, size_t Size> bool LF_HashMap<Value, Size>::insert(const Value& msg)
{
    _epochManager->enterEpoch();

    Bucket* table_p = _table.load();
    size_t index = hash(msg.MessageId, _capacity.load(std::memory_order_acquire));
    Node* newNode = new Node(msg);

    // Traverse the linked list to check for duplicates
    Node* curr = table_p[index].head.load(std::memory_order_acquire);
    while (curr != nullptr)
    {
        if (curr->message.MessageId == msg.MessageId)
        {
            delete newNode;
            std::cout << "message with ID: " << msg.MessageId << " already exist " << "\n";
            _epochManager->exitEpoch();
            return false;
        }

        curr = curr->next.load();
    }

    Node* head = _table[index].head.load(std::memory_order_acquire);
    do
    {
        newNode->next.store(head, std::memory_order_relaxed);
    } while (!_table[index].head.compare_exchange_weak(head, newNode, std::memory_order_release, std::memory_order_relaxed));

    _size.fetch_add(1, std::memory_order_relaxed);
    _epochManager->exitEpoch();

    if (_size.load() >= LOAD_FACTOR * _capacity.load())
    {
        tryResize();
    }

    return true;
}

template <typename Value, size_t Size> void LF_HashMap<Value, Size>::resize()
{
    std::cout << "Resizing the table\n";

    _epochManager->enterEpoch();
    size_t newCapacity = _capacity.load() * 2;

    Bucket* newTable = createBucketArray(newCapacity);
    Bucket* oldTable = _table.load(std::memory_order_acquire);

    // Copy all pointers from the old buckets to the new buckets

    for (size_t i = 0; i < _capacity.load(); ++i)
    {
        Node* curr = oldTable[i].head.load(std::memory_order_acquire);
        while (curr)
        {
            Node* next = curr->next.load(std::memory_order_acquire);
            size_t newIndex = hash(curr->message.MessageId, newCapacity);

            Node* expected = newTable[newIndex].head.load(std::memory_order_acquire);
            do
            {
                curr->next.store(expected, std::memory_order_relaxed);
            } while (!newTable[newIndex].head.compare_exchange_weak(expected, curr, std::memory_order_release, std::memory_order_relaxed));

            curr = next;
        }
    }

    std::atomic_thread_fence(std::memory_order_acq_rel);
    _table.store(newTable, std::memory_order_release);
    _capacity.store(newCapacity, std::memory_order_release);

    // Retire the old table as a custom object
    _epochManager->retireNode(oldTable,
        [](void* ptr)
        {
            Bucket* oldTable = static_cast<Bucket*>(ptr);
            delete[] oldTable;
        }
    );

    _epochManager->exitEpoch();
}

template <typename Value, size_t Size> bool LF_HashMap<Value, Size>::find(uint64_t messageId, Value& result)
{
    _epochManager->enterEpoch();
    size_t index = hash(messageId, _capacity.load());
    Node* curr = _table.load()[index].head.load(std::memory_order_acquire);
    while (curr)
    {
        if (curr->message.MessageId == messageId)
        {
            result = curr->message;
            _epochManager->exitEpoch();
            return true;
        }

        curr = curr->next.load(std::memory_order_acquire);
    }

    _epochManager->exitEpoch();
    return false;
}

template <typename Value, size_t Size> bool LF_HashMap<Value, Size>::remove(uint64_t messageId)
{
    _epochManager->enterEpoch();

    size_t index = hash(messageId, _capacity.load());
    Bucket* table = _table.load();
    Node* prev = nullptr;
    Node* curr = table[index].head.load(std::memory_order_acquire);

    while (curr)
    {
        if (curr->message.MessageId == messageId)
        {
            Node* next = curr->next.load(std::memory_order_acquire);
            if (prev)
            {
                prev->next.store(next, std::memory_order_release);
            }
            else
            {
                table[index].head.store(next, std::memory_order_release);
            }

            _epochManager->retireNode(curr);
            _size.fetch_sub(1, std::memory_order_relaxed);
            _epochManager->exitEpoch();

            return true;
        }

        prev = curr;
        curr = curr->next.load(std::memory_order_acquire);
    }

    _epochManager->exitEpoch();

    return false;
}
