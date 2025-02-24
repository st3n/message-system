#pragma once

#include <message.hpp>

#include <optional>
#include <atomic>

class TcpProcessor
{
  private:
    int _selfPort;

    std::optional<int> init();

    void worker();

    // Static pointer to the current instance
     static TcpProcessor* _instance;
     static void signalHandler(int);

  public:
    TcpProcessor(int port);
    ~TcpProcessor();

    // Delete copy constructor and copy assignment operator
    TcpProcessor(const TcpProcessor&) = delete;
    TcpProcessor& operator=(const TcpProcessor&) = delete;

    TcpProcessor(TcpProcessor&& other) = delete;
    TcpProcessor& operator=(TcpProcessor&& other) = delete;

    void run();
};
