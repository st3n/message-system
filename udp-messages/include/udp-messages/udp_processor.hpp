#pragma once

#include <message.hpp>
#include <messages-container/blocking/hash_map.hpp>

#include <netinet/in.h>
#include <cstddef>
#include <optional>

class UdpProcessor
{
  private:
    int _tcpSockfd{};
    int _sockfd{};
    struct sockaddr_in _servaddr{};
    const char* _tcpServerIp = "127.0.0.1";

    const int _tcpServerPort;
    const int _selfPort;

    HashMap<INITIAL_CAPACITY>& _map;

     // Static pointer to the current instance
     static void signalHandler(int);

    std::optional<int> init();
    void sendViaTcp(Message message);

  public:
    UdpProcessor(int tcpPort, int selfPort, HashMap<INITIAL_CAPACITY>& map);
    ~UdpProcessor();

    UdpProcessor(const UdpProcessor&) = delete;
    UdpProcessor& operator=(const UdpProcessor&) = delete;

    UdpProcessor(UdpProcessor&& other) = delete;
    UdpProcessor& operator=(UdpProcessor&& other) = delete;

    void run();
};
