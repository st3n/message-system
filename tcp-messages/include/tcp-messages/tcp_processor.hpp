#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

class TcpProcessor
{
  public:
    TcpProcessor(uint16_t port);
    ~TcpProcessor();

    void start();
    void stop();

  private:
    bool setupSocket();
    static void signalHandler(int);
    bool addToProcess(int fd);
    void worker();

    uint16_t _port;
    int _serverFd = -1;
    int _epollFd = -1;
    std::thread _workerThread;
};
