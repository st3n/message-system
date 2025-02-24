#pragma once

#include <message.hpp>

void serializeMessage(const Message& msg, char* buffer);
void deserializeMessage(const char* buffer, const Message& msg);

int sendMessage(int sockfd, const Message& msg);
int receiveMessage(int sockfd, Message& msg);

uint64_t ntohll(uint64_t value);
uint64_t htonll(uint64_t value);
