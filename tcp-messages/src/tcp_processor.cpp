#include "tcp-messages/tcp_processor.hpp"
#include "details/ring_buffer.hpp"

#include <message.hpp>
#include <serializer.hpp>

#include <algorithm>
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sys/epoll.h>
#include <unistd.h>

namespace
{

std::mutex epol_mutex{};
std::mutex file_mutex{};
RingBuffer connectionQueue;

std::atomic<bool> _running{false};
}  // namespace

TcpProcessor::TcpProcessor(uint16_t port)
    : _port(port)
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

TcpProcessor::~TcpProcessor()
{
    stop();
}

void TcpProcessor::signalHandler(int signal)
{
    _running.store(false, std::memory_order_release);
    std::cout << "received SIGNAL" << signal << ", stopping TCPProcessor..." << std::endl;
}

void TcpProcessor::start()
{
    if (!setupSocket())
    {
        std::cerr << "Failed to set up socket" << std::endl;
        throw std::runtime_error("Failed to setup TCP socket");
    }

    _running.store(true, std::memory_order_release);
    std::cout << "TCP server started on port " << _port << std::endl;

    _workerThread = std::thread(&TcpProcessor::worker, this);

    while (_running)
    {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientFd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;  // No incoming connections, retry
            }
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            break;
        }

        std::cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port)
                  << std::endl;

        if (!addToProcess(clientFd))
        {
            close(clientFd);
            std::cerr << "Failed to add client to epoll" << std::endl;
        }
    }

    stop();
}

void TcpProcessor::stop()
{
    bool expected = true;
    if (!_running.compare_exchange_strong(expected, false))
        return;

    if (_workerThread.joinable())
    {
        _workerThread.join();
    }

    if (_serverFd > 0)
    {
        close(_serverFd);
        _serverFd = -1;
    }

    if (_epollFd > 0)
    {
        close(_epollFd);
        _epollFd = -1;
    }

    connectionQueue.clear();  // close on invalid fd should not cause UB

    std::cout << "Server stopped" << std::endl;
}

bool TcpProcessor::setupSocket()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
    {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return false;
    }

    /*
    int flags = fcntl(_serverFd, F_GETFL, 0);
    fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK);
    */

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        close(_serverFd);
        return false;
    }

    if (listen(_serverFd, SOMAXCONN) < 0)
    {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(_serverFd);
        return false;
    }

    // Create epoll instance
    _epollFd = epoll_create1(0);
    if (_epollFd < 0)
    {
        std::cerr << "Epoll creation failed: " << strerror(errno) << std::endl;
        close(_serverFd);
        return false;
    }

    return true;
}

bool TcpProcessor::addToProcess(int fd)
{
    if (!connectionQueue.push(fd))
    {
        std::cerr << "Connection queue is full, closing client connection" << std::endl;
        return false;
    }

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd;

    {
        std::unique_lock<std::mutex> lk{epol_mutex};
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) < 0)
        {
            std::cerr << "Epoll_ctl failed: " << strerror(errno) << std::endl;
            return false;
        }
    }

    return true;
}

void TcpProcessor::worker()
{
    const int MAX_EVENTS = 16;
    epoll_event events[MAX_EVENTS];

    while (_running)
    {
        int numEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
        if (numEvents < 0)
        {
            if (errno == EINTR)
                continue;  // Retry if interrupted
            std::cerr << "Epoll_wait failed: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < numEvents; i++)
        {
            int fd = events[i].data.fd;

            Message receivedMessage;
            ssize_t bytesRead = receiveMessage(fd, receivedMessage);
            if (bytesRead == sizeof(receivedMessage))
            {
                std::cout << "Received TCP message: Type=" << (int)receivedMessage.MessageType
                          << ", Id=" << receivedMessage.MessageId << ", Data=" << receivedMessage.MessageData
                          << std::endl;
                          /*
                {
                    std::unique_lock<std::mutex> lk(file_mutex);
                    std::ofstream log_file("tcp_messaages.log", std::ios::app);
                    log_file << "Size: " << receivedMessage.MessageSize << " Type: " << receivedMessage.MessageType
                             << " ID: " << receivedMessage.MessageId << " Data: " << receivedMessage.MessageData
                             << std::endl;
                }
                             */
            }
            else if (bytesRead == 0 || (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
            {
                // Client disconnected
                std::cout << "Client disconnected: " << fd << std::endl;
                close(fd);
                std::unique_lock<std::mutex> lock(epol_mutex);  // Guard epoll_fd
                epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);
            }
        }
    }
}
