#include "tcp-messages/tcp_processor.hpp"
#include "details/ring_buffer.hpp"

#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <mutex>

namespace
{

int sockfd{};
struct sockaddr_in servaddr{};

std::atomic<bool> _running{false};
constexpr int WORKER_COUNT = 12;
std::thread workers[WORKER_COUNT];

RingBuffer connectionQueue;
std::mutex file_mutex;
}  // namespace

TcpProcessor::TcpProcessor(int port)
    : _selfPort{port}
{
    _running.store(true, std::memory_order_release);

    if (_selfPort < 0 || _selfPort > 65535)
    {
        throw std::invalid_argument{"Invalid port number"};
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

TcpProcessor::~TcpProcessor()
{
    _running.store(false, std::memory_order_release);

    for (int i = 0; i < WORKER_COUNT; i++)
    {
        if (workers[i].joinable())
        {
            workers[i].join();
        }
    }

    if (sockfd > 0)
    {
        close(sockfd);
    }
}

std::optional<int> TcpProcessor::init()
{
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "TCP socket creation failed" << std::endl;
        return std::nullopt;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(_selfPort);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr << "TCP bind failed" << std::endl;
        return std::nullopt;
    }

    if (listen(sockfd, 1000) < 0)
    {
        std::cerr << "TCP listen failed" << std::endl;
        return std::nullopt;
    }

    return sockfd;
}

void TcpProcessor::run()
{
    if (!init())
    {
        throw std::runtime_error("Failed to create TCP socket");
    }

    std::cout << "TCP server started on port " << _selfPort << std::endl;

    for (int i = 0; i < WORKER_COUNT; i++)
    {
        workers[i] = std::thread(&TcpProcessor::worker, this);
    }

    while (_running.load(std::memory_order_acquire))
    {
        sockaddr_in cliaddr{};
        socklen_t len = sizeof(cliaddr);
        int connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &len);
        if (connfd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            std::cerr << "TCP accept failed: " << strerror(errno) << std::endl;
            break;
        }

        if (!connectionQueue.push(connfd))
        {
            std::cerr << "Connection queue is full, closing client connection" << std::endl;
            close(connfd);
        }
    }

    std::cout << "TCP server stopped" << std::endl;
}

void TcpProcessor::worker()
{
    while (_running.load(std::memory_order_acquire))
    {
        int clientFd;
        if (connectionQueue.pop(clientFd))
        {
            Message receivedMessage;
            while (true)
            {
                ssize_t received = recv(clientFd, &receivedMessage, sizeof(receivedMessage), 0);
                if (received == sizeof(receivedMessage))
                {
                    std::cout << "Received TCP message: Type=" << (int)receivedMessage.MessageType
                              << ", Id=" << receivedMessage.MessageId << ", Data=" << receivedMessage.MessageData
                              << std::endl;
                    {
                        std::lock_guard<std::mutex> lock(file_mutex);
                            std::ofstream log_file("tcp_messaages.log", std::ios::app);
                            log_file << "Size: " << receivedMessage.MessageSize
                                     << " Type: " << receivedMessage.MessageType << " ID: " << receivedMessage.MessageId
                                     << " Data: " << receivedMessage.MessageData << std::endl;
                    }
                }
                else if (received == 0)
                {
                    break;  // Client disconnected
                }
                else if (received < 0)
                {
                    std::cerr << "TCP recv failed: " << strerror(errno) << std::endl;
                    break;
                }
            }

            close(clientFd);
        }

        std::this_thread::yield();  // Avoid busy waiting
    }
}

void TcpProcessor::signalHandler(int)
{
    _running.store(false, std::memory_order_release);
    std::cout << "SIGINT received, stopping UdpProcessor..." << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <TCP_PORT> " << std::endl;
        return 1;
    }

    int tcpPort = std::stoi(argv[1]);

    TcpProcessor tcpProcessor(tcpPort);
    tcpProcessor.run();

    return 0;
}
