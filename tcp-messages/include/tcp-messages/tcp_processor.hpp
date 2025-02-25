#pragma once

#include <sys/epoll.h>
#include <atomic>


class TcpServer
{
  public:
    TcpServer(int port);
    ~TcpServer();
    void run();

  private:
    int _serverFd;
    int _epollFd;
    int _port;
    volatile static std::atomic_bool _running;
    int _clientFds[FD_SETSIZE];
    int _clientCount;


    bool setupServer();
    void handleConnections();
    static void signalHandler(int);
    static int makeNonBlocking(int fd);
    void closeClients();
};
