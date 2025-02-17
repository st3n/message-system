#pragma once

#include "message.hpp"
#include <pthread.h>

class MessageContainer
{
public:
    MessageContainer();
    ~MessageContainer();

    bool insert(const Message& message);
    bool contains(uint64_t messageId) const;

private:
    static const size_t CONTAINER_SIZE = 1000;

    Message messages[CONTAINER_SIZE];
    bool occupied[CONTAINER_SIZE];
};
