#include <messages-container/lock_free_container.hpp>

#include <iostream>
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

size_t LockFreeMessageMap::hash(uint64_t messageId) const {
    messageId = ((messageId >> 32) ^ messageId) * 0x45d9f3b; // Mix high and low bits
    messageId = ((messageId >> 16) ^ messageId) * 0x45d9f3b; // Further mix
    messageId = (messageId >> 16) ^ messageId; // Final mix
    return messageId % BUCKET_COUNT; // Map to a bucket
}

LockFreeMessageMap::LockFreeMessageMap() {
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        _buckets[i].store(nullptr, std::memory_order_relaxed);
    }
}

LockFreeMessageMap::~LockFreeMessageMap() {
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        Node* curr = _buckets[i].load(std::memory_order_acquire);
        while (curr != nullptr) {
            Node* next = curr->_next.load(std::memory_order_acquire);
            delete curr;
            curr = next;
        }
    }
}

bool LockFreeMessageMap::insert(const Message& msg) {
    size_t index = hash(msg.MessageId);
    Node* newNode = new Node(msg);
    Node* head = _buckets[index].load(std::memory_order_acquire);

    Node* curr = head;
    while (curr != nullptr) {
        if (curr->_message.MessageId == msg.MessageId) {
            delete newNode;
            return false;
        }
        curr = curr->_next.load(std::memory_order_acquire);
    }

    newNode->_next.store(head, std::memory_order_relaxed);
    while (!_buckets[index].compare_exchange_weak(head, newNode, std::memory_order_release, std::memory_order_relaxed)) {
        newNode->_next.store(head, std::memory_order_relaxed);
    }

    return true;
}

Message* LockFreeMessageMap::find(uint64_t messageId) {
    size_t index = hash(messageId);
    Node* curr = _buckets[index].load(std::memory_order_acquire);

    while (curr != nullptr) {
        if (curr->_message.MessageId == messageId) {
            return &curr->_message;
        }
        curr = curr->_next.load(std::memory_order_acquire);
    }

    return nullptr;
}

void LockFreeMessageMap::remove(uint64_t messageId) {
    size_t index = hash(messageId);
    Node* prev = nullptr;
    Node* curr = _buckets[index].load(std::memory_order_acquire);

    while (curr != nullptr) {
        if (curr->_message.MessageId == messageId) {
            Node* next = curr->_next.load(std::memory_order_acquire);

            if (prev == nullptr) {
                if (_buckets[index].compare_exchange_weak(curr, next, std::memory_order_release, std::memory_order_relaxed)) {
                    delete curr;
                    return;
                }
            } else {
                prev->_next.store(next, std::memory_order_release);
                delete curr;
                return;
            }
        }

        prev = curr;
        curr = curr->_next.load(std::memory_order_acquire);
    }
}
