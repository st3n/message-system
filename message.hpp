#pragma once

#include <cstdint>

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
};

constexpr uint16_t MESSAGE_SIZE = sizeof(Message);
constexpr uint16_t UDP_PORT_1   = 50001;
constexpr uint16_t UDP_PORT_2   = 50002;
constexpr uint16_t TCP_PORT     = 50003;
