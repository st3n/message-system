#pragma once

#include <message.hpp>
#include <messages-container/blocking/hash_map.hpp>

#include <cstddef>
#include <thread>
#include <optional>

constexpr size_t INITIAL_CAPACITY = 1024;

class UdpProcessor
{
  private:
    const char* _tcpServerIp = "127.0.0.1";

    const int _tcpServerPort;
    const int _udpServerPort;
    const int _selfPort;

    HashMap<INITIAL_CAPACITY>& _map;
    std::thread _thread;
    std::atomic<bool> _running;

    std::optional<int> init();
    void sendViaTcp(Message message);

  public:
    UdpProcessor(int tcpPort, int udpPort, int selfPort, HashMap<INITIAL_CAPACITY>& map);
    ~UdpProcessor();

    UdpProcessor(const UdpProcessor&) = delete;
    UdpProcessor& operator=(const UdpProcessor&) = delete;

    UdpProcessor(UdpProcessor&& other) = delete;
    UdpProcessor& operator=(UdpProcessor&& other) = delete;

    void run();
};
