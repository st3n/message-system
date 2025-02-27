#pragma once

#include <cstdint>
#include <cstddef>

struct Message
{
    uint16_t MessageSize;
    uint8_t MessageType;
    uint64_t MessageId;
    uint64_t MessageData;

    bool operator<(const Message& other) const
    {
        return MessageType < other.MessageType;
    }

    bool operator==(const Message& other) const
    {
        return MessageId == other.MessageId;
    }
};

constexpr uint16_t MESSAGE_SIZE = sizeof(Message);

constexpr size_t INITIAL_CAPACITY = 1024;
