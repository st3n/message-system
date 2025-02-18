#pragma once

#include "message.hpp"

#include <atomic>
#include <cstdio>

struct Node
{
    Message _message;
    std::atomic<Node*> _next;
    Node(const Message& msg)
        : _message(msg)
        , _next(nullptr)
    {
    }
};

/// @brief: Lock-free hashmap
class LockFreeMessageMap
{
  private:
    static const size_t BUCKET_COUNT = 8191;  // Large prime number for better distribution
    std::atomic<Node*> _buckets[BUCKET_COUNT];

    /// @brief  Hash function to map a message ID to a bucket index
    /// Probably there could be a better hash function
    /// @param messageId
    /// @return size_t
    size_t hash(uint64_t messageId) const;

  public:
    LockFreeMessageMap();
    LockFreeMessageMap(const LockFreeMessageMap&) = delete;

    ~LockFreeMessageMap();

    // Insert a message (drop duplicates)
    bool insert(const Message& msg);

    // Find a message by MessageId
    Message* find(uint64_t messageId);

    // Remove a message by MessageId
    void remove(uint64_t messageId);
};
