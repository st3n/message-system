#include "udp-messages/udp_processor.hpp"

#include <serializer.hpp>
#include <common/signal_handler.hpp>

#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <mutex>


namespace
{

std::mutex file_mutex;

}  // namespace

UdpServer::UdpServer(int tcpPort, int selfPort, HashMap<INITIAL_CAPACITY>& map)
    : _tcpServerPort(tcpPort)
    , _selfPort(selfPort)
    , _map(map)
{
    const std::array<int, 3> temp = {_tcpServerPort, _selfPort};

    for (const auto port : temp)
    {
        if (port < 0 || port > 65535)
        {
            throw std::invalid_argument("Invalid port number");
        }
    }
}

UdpServer::~UdpServer()
{
    _running.store(false, std::memory_order_release);

    if (_tcpSockfd > 0)
    {
        close(_tcpSockfd);
    }

    if (_sockfd > 0)
    {
        close(_sockfd);
    }
}

std::optional<int> UdpServer::init()
{
    if ((_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return std::nullopt;
    }

    // Set socket to non-blocking mode
    int flags{fcntl(_sockfd, F_GETFL, 0)};
    if (flags == -1)
    {
        std::cerr << "Failed to get socket flags" << std::endl;
        return std::nullopt;
    }

    if (fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        std::cerr << "Failed to set socket to non-blocking mode" << std::endl;
        return std::nullopt;
    }

    memset(&_servaddr, 0, sizeof(_servaddr));

    _servaddr.sin_family = AF_INET;
    _servaddr.sin_addr.s_addr = INADDR_ANY;
    _servaddr.sin_port = htons(_selfPort);

    if (bind(_sockfd, (const struct sockaddr*)&_servaddr, sizeof(_servaddr)) < 0)
    {
        std::cerr << "Bind failed" << std::endl;
        return std::nullopt;
    }

    // open and setup tcp socket:
    struct sockaddr_in tcpserverAddr{};

    if ((_tcpSockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "TCP socket creation failed" << std::endl;
        return std::nullopt;
    }

    memset(&tcpserverAddr, 0, sizeof(tcpserverAddr));
    tcpserverAddr.sin_family = AF_INET;
    tcpserverAddr.sin_port = htons(_tcpServerPort);

    if (inet_pton(AF_INET, _tcpServerIp, &tcpserverAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid TCP server address" << std::endl;
        return std::nullopt;
    }

    if (connect(_tcpSockfd, (struct sockaddr*)&tcpserverAddr, sizeof(tcpserverAddr)) < 0)
    {
        std::cerr << "TCP connection failed" << std::endl;
        return std::nullopt;
    }

    return _sockfd;  // return udp sock
}

void UdpServer::run()
{
    if (!init())
    {
        throw std::runtime_error("Failed to create UDP socket");
    }

    std::cout << "UDP server started on port " << _selfPort << std::endl;

    fd_set read_fds{};
    struct timeval timeout{};

    while (_running.load(std::memory_order_acquire))
    {
        FD_ZERO(&read_fds);
        FD_SET(_sockfd, &read_fds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 500;

        int activity{select(_sockfd + 1, &read_fds, nullptr, nullptr, &timeout)};
        if (activity < 0)
        {
            std::cerr << "select failed: " << strerror(errno) << std::endl;
            break;
        }
        else if (activity == 0)
        {
            // Timeout occurred, no data available
            continue;
        }

        if (FD_ISSET(_sockfd, &read_fds))
        {
            Message receivedMessage{};
            int n = receiveMessage(_sockfd, receivedMessage);

            if (n == sizeof(receivedMessage))
            {
                std::cout << "Received message: Type=" << static_cast<int>(receivedMessage.MessageType)
                          << ", Id=" << receivedMessage.MessageId << ", Data=" << receivedMessage.MessageData
                          << std::endl;

                _map.insert(receivedMessage);  // duplicates are managed inside container

                // If MessageData equals 10, send it via TCP asynchronously
                if (receivedMessage.MessageData == 10)
                {
                    /*
                    std::lock_guard<std::mutex> lock(file_mutex);
                    std::ofstream log_file("udp_messaages.log", std::ios::app);
                    log_file << "Size: " << receivedMessage.MessageSize << " Type: " << receivedMessage.MessageType << " ID: " << receivedMessage.MessageId
                             << " Data: " << receivedMessage.MessageData << std::endl;

                             */
                    std::jthread(&UdpServer::sendViaTcp, this, receivedMessage).detach();
                }
            }
            else if (n < 0)
            {
                std::cerr << "recvfrom failed: " << strerror(errno) << std::endl;
            }
        }
    }

    std::cout << "UDP server stopped" << std::endl;
}

void UdpServer::sendViaTcp(Message message)
{
    int n = sendMessage(_tcpSockfd, message);
    if (n < 0)
    {
        std::cerr << "Failed to send message via TCP" << std::endl;
    }
}
