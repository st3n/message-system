#include "serializer.hpp"

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>

void serializeMessage(const Message& msg, char* buffer)
{
    uint16_t size = htons(msg.MessageSize);
    uint64_t id = htonll(msg.MessageId);
    uint64_t data = htonll(msg.MessageData);

    memcpy(buffer, &size, sizeof(size));
    memcpy(buffer + sizeof(size), &msg.MessageType, sizeof(msg.MessageType));
    memcpy(buffer + sizeof(size) + sizeof(msg.MessageType), &id, sizeof(id));
    memcpy(buffer + sizeof(size) + sizeof(msg.MessageType) + sizeof(id), &data, sizeof(data));
}

void deserializeMessage(const char* buffer, Message& msg)
{
    uint16_t size;
    uint64_t id;
    uint64_t data;

    memcpy(&size, buffer, sizeof(size));
    memcpy(&msg.MessageType, buffer + sizeof(size), sizeof(msg.MessageType));
    memcpy(&id, buffer + sizeof(size) + sizeof(msg.MessageType), sizeof(id));
    memcpy(&data, buffer + sizeof(size) + sizeof(msg.MessageType) + sizeof(id), sizeof(data));

    msg.MessageSize = ntohs(size);
    msg.MessageId = ntohll(id);
    msg.MessageData = ntohll(data);
}

uint64_t htonll(uint64_t value)
{
    static const int num = 1;
    if (*reinterpret_cast<const char*>(&num) == 1)
    {
        // Little-endian system
        uint32_t high = htonl(static_cast<uint32_t>(value >> 32));
        uint32_t low = htonl(static_cast<uint32_t>(value & 0xFFFFFFFF));
        return (static_cast<uint64_t>(low) << 32) | high;
    }
    else
    {
        // Big-endian system, no conversion needed
        return value;
    }
}

uint64_t ntohll(uint64_t value)
{
    return htonll(value);
}

int sendMessage(int sockfd, const Message& msg)
{
    char buffer[sizeof(Message)];
    serializeMessage(msg, buffer);

    size_t totalSent = 0;
    size_t messageSize = sizeof(Message);

    while (totalSent < messageSize)
    {
        ssize_t bytesSent = send(sockfd, buffer + totalSent, messageSize - totalSent, 0);

        if (bytesSent < 0)
        {
            std::cerr << "Send failed: " << strerror(errno) << std::endl;
            return -1;
        }

        totalSent += bytesSent;
    }

    return totalSent;
}

int receiveMessage(int sockfd, Message& msg)
{
    char buffer[sizeof(Message)];
    size_t totalReceived = 0;

    while (totalReceived < sizeof(Message))
    {
        ssize_t bytesRead = recv(sockfd, buffer + totalReceived, sizeof(Message) - totalReceived, 0);

        if (bytesRead < 0)
        {
            std::cerr << "Receive failed: " << strerror(errno) << std::endl;
            return -1;
        }
        else if (bytesRead == 0)
        {
            std::cerr << "Connection closed by peer" << std::endl;
            return -1;
        }

        totalReceived += bytesRead;
    }

    deserializeMessage(buffer, msg);

    return totalReceived;
}
