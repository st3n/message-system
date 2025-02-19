#pragma once

#include "message.hpp"

#include <atomic>
#include <cstdio>

/// @brief: Lock-free hashmap with dynamic resizing and linear probing
/// The map is implemented as an array of buckets, each bucket is a linked list of nodes
/// Each node contains a message and a pointer to the next node
/// The map is lock-free, but not wait-free, because resizing is done with CAS
/// Could be done as template class -> template <size_t BUCKET_COUNT = 8191>
class LockFreeMessageMap
{
    struct Node
    {
        Message message;
        std::atomic<Node*> next;

        Node(const Message& msg)
            : message(msg)
            , next(nullptr)
        {
        }
    };

    struct Bucket
    {
        std::atomic<Node*> head;
        Bucket()
            : head(nullptr)
        {
        }
    };

    ///@NOTE: atomic<shared_ptr<Node>> would be better, but C++20 standard impl is not lock free
    /// there are other not commonly used implementation, but they are very platform specific
    /// so let's stick with atomic<Node*> for now, raw pointers, is not  perfect, but it's lock free

    std::atomic<Bucket*> _table{nullptr};
    std::atomic<size_t> _size{0}; // Current number of elements
    std::atomic<size_t> _capacity;
    std::atomic<bool> _resizing{false};

    static constexpr float LOAD_FACTOR = 0.75f;

    /// @brief  Hash function to map a message ID to a bucket index
    /// Probably there could be a better hash function and it worse to use std::hash
    /// @param messageId
    /// @param bucket_count
    /// @return size_t
    size_t hash(uint64_t messageId, size_t bucket_count) const;

    Bucket* createBucketArray(size_t newSize);
    void resize();

  public:

    void debug() const {
        for (size_t i = 0; i < _capacity; ++i) {
            printf("Bucket %zu: ", i);
            Node* node = _table[i].head.load();
            while (node) {
                printf("%lu -> ", node->message.MessageId);
                node = node->next.load();
            }
            printf("nullptr\n");
        }
    }

    LockFreeMessageMap(size_t initial_capacity);
    ~LockFreeMessageMap();

    LockFreeMessageMap(const LockFreeMessageMap&)            = delete;
    LockFreeMessageMap& operator=(const LockFreeMessageMap&) = delete;

    // Insert a message (drop duplicates)
    bool insert(const Message& msg);

    // Find a message by MessageId
    bool find(uint64_t messageId, Message& result);

    // Remove a message by MessageId
    bool remove(uint64_t messageId);

    // Get the current number of elements
    size_t size() const;
};
