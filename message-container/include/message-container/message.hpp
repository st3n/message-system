#pragma once

#include <cstdint>

struct Message {
    uint16_t MessageSize;
    uint8_t MessageType;
    uint64_t MessageId;
    uint64_t MessageData;
};

const uint16_t MAX_MESSAGE_SIZE = sizeof(Message);
const uint16_t UDP_PORT_1 = 50001;
const uint16_t UDP_PORT_2 = 50002;
const uint16_t TCP_PORT = 50003;
