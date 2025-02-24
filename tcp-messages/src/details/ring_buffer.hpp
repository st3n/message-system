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
        int next = (head.load(std::memory_order_relaxed) + 1) % BUFFER_SIZE;
        if (next == tail.load(std::memory_order_acquire))
        {
            return false;  // Buffer full
        }

        buffer[head] = fd;
        head.store(next, std::memory_order_release);

        return true;
    }

    bool pop(int& fd)
    {
        if (head.load(std::memory_order_acquire) == tail.load(std::memory_order_relaxed))
        {
            return false;  // Buffer empty
        }

        fd = buffer[tail];
        tail.store((tail.load(std::memory_order_relaxed) + 1) % 1024, std::memory_order_release);

        return true;
    }
};
