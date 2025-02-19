#include <messages-container/lock_free_container.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>

LockFreeMessageMap::LockFreeMessageMap(size_t initial_capacity)
    : _capacity(initial_capacity)
{
    _table.store(createBucketArray(initial_capacity), std::memory_order_release);
}

LockFreeMessageMap::~LockFreeMessageMap()
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

    delete[] table_p;
}

LockFreeMessageMap::Bucket* LockFreeMessageMap::createBucketArray(size_t newSize)
{
    Bucket* newBuckets = new Bucket[newSize];
    for (size_t i = 0; i < newSize; ++i)
    {
        newBuckets[i].head.store(nullptr, std::memory_order_relaxed);
    }

    return newBuckets;
}

size_t LockFreeMessageMap::hash(uint64_t messageId, size_t bucket_count) const
{
    return std::hash<uint64_t>{}(messageId) % bucket_count;
}

bool LockFreeMessageMap::insert(const Message& msg)
{
    std::cout << "Inserting message with ID: " << msg.MessageId << "\n";

    Bucket* table_p = _table.load();
    size_t index    = hash(msg.MessageId, _capacity.load(std::memory_order_acquire));

    Node* newNode = new Node(msg);

    // Traverse the linked list to check for duplicates
    Node* curr = table_p[index].head.load(std::memory_order_acquire);
    while (curr != nullptr)
    {
        if (curr->message.MessageId == msg.MessageId)
        {
            delete newNode;
            std::cout << "message with ID: " << msg.MessageId << "already exist " << "\n";
            return false;
        }

        curr = curr->next.load();
    }

    Node* idx_head_p = table_p[index].head.load();
    newNode->next.store(idx_head_p, std::memory_order_relaxed);
    while (!table_p[index].head.compare_exchange_strong(
        idx_head_p, newNode, std::memory_order_release, std::memory_order_relaxed))
    {
        newNode->next.store(idx_head_p, std::memory_order_relaxed);
    }

    auto size = _size.fetch_add(1) + 1;

    if (size >= LOAD_FACTOR * _capacity.load(std::memory_order_acquire))
    {
        resize();
    }


    std::cout << "message with ID: " << msg.MessageId << " inserted" << "\n";
    return true;
}

void LockFreeMessageMap::resize()
{
    if (_resizing.exchange(true, std::memory_order_acquire))
    {
        return;
    }

    size_t oldCapacity = _capacity.load();
    size_t newCapacity = oldCapacity * 2;

    Bucket* oldTable = _table.load(std::memory_order_acquire);
    Bucket* newTable = createBucketArray(newCapacity);

    // Copy all pointers from the old buckets to the new buckets
    for (size_t i = 0; i < oldCapacity; ++i)
    {
        Node* curr = oldTable[i].head.load(std::memory_order_acquire);
        while (curr != nullptr)
        {
            Node* next      = curr->next.load(std::memory_order_acquire);
            size_t newIndex = hash(curr->message.MessageId, newCapacity);

            Node* expected = newTable[newIndex].head.load(std::memory_order_acquire);
            curr->next.store(expected, std::memory_order_relaxed);

            while (!newTable[newIndex].head.compare_exchange_strong(
                expected, curr, std::memory_order_release, std::memory_order_relaxed))
            {
                curr->next.store(expected, std::memory_order_relaxed);
            }

            curr = next;
        }
    }

    _capacity.store(newCapacity);
    _table.store(newTable);
    delete[] oldTable;

    _resizing.store(false, std::memory_order_release);
}

bool LockFreeMessageMap::find(uint64_t messageId, Message& result)
{
    std::cout << "Finding message with ID: " << messageId << "\n";

    Bucket* tablePtr = _table.load(std::memory_order_acquire);
    size_t index       = hash(messageId, _capacity.load(std::memory_order_acquire));
    Node* curr         = tablePtr[index].head.load(std::memory_order_acquire);

    while (curr != nullptr)
    {
        if (curr->message.MessageId == messageId)
        {
            result = curr->message;
            return true;
        }

        curr = curr->next.load(std::memory_order_acquire);
    }

    return false;
}

bool LockFreeMessageMap::remove(uint64_t messageId)
{
    std::cout << "Removing message with ID: " << messageId << "\n";

    Bucket* tablePtr = _table.load(std::memory_order_acquire);
    size_t index       = hash(messageId, _capacity.load(std::memory_order_acquire));
    Node* prev         = nullptr;
    Node* curr         = tablePtr[index].head.load();

    while (curr != nullptr)
    {
        Node* next = curr->next.load(std::memory_order_acquire);

        if (curr->message.MessageId == messageId)
        {
            std::cout << "message with ID: " << messageId << " ready to be removed\n" ;
            if (prev == nullptr)
            {
                if (tablePtr[index].head.compare_exchange_strong(
                    curr, next, std::memory_order_release, std::memory_order_relaxed))
                {
                    delete curr;
                    _size.fetch_sub(1);
                    std::cout << "message with ID: " << messageId << " removed\n" ;
                    return true;
                }
            }
            else
            {
                if (prev->next.compare_exchange_strong(
                    curr, next, std::memory_order_release, std::memory_order_relaxed))
                {
                    delete curr;
                    _size.fetch_sub(1);
                    std::cout << "message with ID: " << messageId << " removed\n" ;
                    return true;
                }
            }
        }
        else
        {
            prev = curr;
            curr = next;
        }
    }

    return false;
}

size_t LockFreeMessageMap::size() const
{
    return _size.load(std::memory_order_acquire);
}
