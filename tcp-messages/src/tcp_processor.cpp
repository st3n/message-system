#include "tcp-messages/tcp_processor.hpp"
#include <message.hpp>
#include <serializer.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <mutex>
#include <fstream>

namespace
{

std::mutex file_mutex;
constexpr int MAX_EVENTS = 16;

}  // namespace

volatile std::atomic_bool TcpServer::_running = true;

TcpServer::TcpServer(int port)
    : _port(port)
    , _clientCount(0)
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (!setupServer())
    {
        throw std::runtime_error("Couldn't setup a socket");
    }
}

TcpServer::~TcpServer()
{
    closeClients();

    close(_serverFd);
    close(_epollFd);
}

void TcpServer::signalHandler(int)
{
    _running = false;
    std::cout << "SIGINT received, stopping TCPServer..." << std::endl;
}

int TcpServer::makeNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool TcpServer::setupServer()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
    {
        std::cerr << "Socket creation failed\n";
        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Bind failed\n";
        return false;
    }

    if (listen(_serverFd, SOMAXCONN) < 0)
    {
        std::cerr << "Listen failed\n";
        return false;
    }

    makeNonBlocking(_serverFd);

    _epollFd = epoll_create1(0);
    if (_epollFd < 0)
    {
        std::cerr << "Epoll creation failed\n";
        return false;
    }

    epoll_event event = {};
    event.events = EPOLLIN;
    event.data.fd = _serverFd;
    epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverFd, &event);

    return true;
}

void TcpServer::run()
{
    epoll_event events[MAX_EVENTS];

    while (_running)
    {
        int numEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
        if (numEvents < 0)
            continue;

        for (int i = 0; i < numEvents; ++i)
        {
            int fd = events[i].data.fd;
            if (fd == _serverFd)
            {
                sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientFd > 2)
                {
                    char clientIP[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
                    std::cout << "Client connected: " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;

                    makeNonBlocking(clientFd);
                    epoll_event event = {};
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = clientFd;
                    epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &event);
                    _clientFds[_clientCount++] = clientFd;
                }
            }
            else
            {
                Message receivedMessage;
                ssize_t bytesRead = receiveMessage(fd, receivedMessage);
                if (bytesRead == sizeof(receivedMessage))
                {
                    std::cout << "Received TCP message: Type=" << (int)receivedMessage.MessageType
                              << ", Id=" << receivedMessage.MessageId << ", Data=" << receivedMessage.MessageData
                              << std::endl;

                    if (receivedMessage.MessageData == 10)
                    {
                        std::unique_lock<std::mutex> lk(file_mutex);
                        std::ofstream log_file("tcp_messaages.log", std::ios::app);
                        log_file << "Size: " << receivedMessage.MessageSize << " Type: " << receivedMessage.MessageType
                                 << " ID: " << receivedMessage.MessageId << " Data: " << receivedMessage.MessageData
                                 << std::endl;
                    }
                }
                else if (bytesRead == 0 || (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
                {
                    // Client disconnected
                    std::cout << "Client disconnected: " << fd << std::endl;
                    close(fd);
                    epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);
                    for (int j = 0; j < _clientCount; ++j)
                    {
                        if (_clientFds[j] == fd)
                        {
                            _clientFds[j] = 0;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void TcpServer::closeClients()
{
    for (int i = 0; i < _clientCount; ++i)
    {
        if (_clientFds[i] > 2)
            close(_clientFds[i]);
    }

    _clientCount = 0;
}
