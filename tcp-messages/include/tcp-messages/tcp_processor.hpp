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
    int _clientFds[FD_SETSIZE];
    int _clientCount;


    bool setupServer();
    void handleConnections();
    static int makeNonBlocking(int fd);
    void closeClients();
};
