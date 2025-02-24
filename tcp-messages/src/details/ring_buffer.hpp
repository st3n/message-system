#pragma once

#include <atomic>

constexpr int BUFFER_SIZE = 1024;

// Lock-free ring buffer for client connections
struct RingBuffer
{
    int buffer[BUFFER_SIZE];
    std::atomic<int> head{0};
    std::atomic<int> tail{0};

    bool push(int fd)
    {
        int current_head = head.load(std::memory_order_relaxed);
        int next = (current_head + 1) % BUFFER_SIZE;

        if (next == tail.load(std::memory_order_acquire))
        {
            return false;  // Buffer full
        }

        buffer[current_head] = fd;
        head.store(next, std::memory_order_release);

        return true;
    }

    bool pop(int& fd)
    {
        int current_tail = tail.load(std::memory_order_relaxed);
        if (current_tail == head.load(std::memory_order_acquire))
        {
            return false;  // Buffer empty
        }

        fd = buffer[current_tail];
        tail.store((current_tail + 1) % BUFFER_SIZE, std::memory_order_release);

        return true;
    }

    bool empty() const
    {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

    /// @brief should nor be called cuncurently with push and pop
    void clear()
    {
        int current_tail = tail.load(std::memory_order_relaxed);
        int current_head = head.load(std::memory_order_relaxed);

        while (current_tail != current_head)
        {
            close(buffer[current_tail]);
            current_tail = (current_tail + 1) % BUFFER_SIZE;
        }

        tail.store(current_tail, std::memory_order_release);
    }
};
