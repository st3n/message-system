#pragma once

#include <messages-container/blocking/hash_map.hpp>

#include <netinet/in.h>
#include <cstddef>
#include <optional>

class UdpServer
{
  private:
    int _tcpSockfd{};
    int _sockfd{};
    struct sockaddr_in _servaddr{};
    const char* _tcpServerIp = "127.0.0.1";

    const int _tcpServerPort;
    const int _selfPort;

    HashMap<INITIAL_CAPACITY>& _map;

    std::optional<int> init();
    void sendViaTcp(Message message);

  public:
    UdpServer(int tcpPort, int selfPort, HashMap<INITIAL_CAPACITY>& map);
    ~UdpServer();

    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;

    UdpServer(UdpServer&& other) = delete;
    UdpServer& operator=(UdpServer&& other) = delete;

    void run();
};
