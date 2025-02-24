#pragma once

#include <atomic>
#include <memory>

class Spinlock
{
  public:
    Spinlock()
        : _flag(std::make_shared<std::atomic_flag>())
    {
        _flag->clear();
    }

    // Copy constructor (shared state)
    Spinlock(const Spinlock& other)
        : _flag(other._flag)
    {
    }

    // Move constructor (transfer ownership)
    Spinlock(Spinlock&& other) noexcept
        : _flag(std::move(other._flag))
    {
    }

    Spinlock& operator=(const Spinlock& other)
    {
        if (this != &other)
        {
            _flag = other._flag;
        }

        return *this;
    }

    Spinlock& operator=(Spinlock&& other) noexcept
    {
        if (this != &other)
        {
            _flag = std::move(other._flag);
        }

        return *this;
    }

    bool try_lock() noexcept
    {
        return !_flag->test_and_set(std::memory_order_acquire);
    }

    // Lock (spin until available)
    void lock()
    {
        while (_flag->test_and_set(std::memory_order_acquire))
        {
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L
            _flag->wait(true, std::memory_order_relaxed);
#else
            while (_flag->test(std::memory_order_relaxed))
                ;  // Reduce contention
#endif
        }
    }

    void unlock()
    {
        _flag->clear(std::memory_order_release);
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L
        _flag->notify_all();
#endif
    }

  private:
    std::shared_ptr<std::atomic_flag> _flag;
};
