#include <memory>
#include <shared_mutex>

class SharedMutex
{
  public:
    SharedMutex()
        : _mutex(std::make_shared<std::shared_mutex>())
    {
    }

    // Copy constructor (shared ownership)
    SharedMutex(const SharedMutex& other)
        : _mutex(other._mutex)
    {
    }

    // Move constructor (transfer ownership)
    SharedMutex(SharedMutex&& other) noexcept
        : _mutex(std::move(other._mutex))
    {
    }

    SharedMutex& operator=(const SharedMutex& other)
    {
        if (this != &other)
        {
            _mutex = other._mutex;
        }
        return *this;
    }

    SharedMutex& operator=(SharedMutex&& other) noexcept
    {
        if (this != &other)
        {
            _mutex = std::move(other._mutex);
        }
        return *this;
    }

    // Lock for exclusive access (writers)
    void lock()
    {
        _mutex->lock();
    }

    // Try locking for exclusive access
    bool try_lock()
    {
        return _mutex->try_lock();
    }

    // Unlock exclusive access
    void unlock()
    {
        _mutex->unlock();
    }

    // Lock for shared access (readers)
    void lock_shared()
    {
        _mutex->lock_shared();
    }

    // Try locking for shared access
    bool try_lock_shared()
    {
        return _mutex->try_lock_shared();
    }

    // Unlock shared access
    void unlock_shared()
    {
        _mutex->unlock_shared();
    }

  private:
    std::shared_ptr<std::shared_mutex> _mutex;
};
