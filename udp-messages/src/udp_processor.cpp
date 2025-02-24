#include "udp-messages/udp_processor.hpp"

#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{

int selfSockfd{};
int tcpSockfd{};
struct sockaddr_in servaddr{}, cliaddr{};
socklen_t len{sizeof(cliaddr)};

}  // namespace

UdpProcessor::UdpProcessor(int tcpPort, int udpPort, int selfPort, HashMap<INITIAL_CAPACITY>& map)
    : _tcpServerPort(tcpPort)
    , _udpServerPort(udpPort)
    , _selfPort(selfPort)
    , _map(map)
    , _running(true)
{
    const std::array<int, 3> temp = {_tcpServerPort, _udpServerPort, _selfPort};

    for (const auto port : temp)
    {
        if (port < 0 || port > 65535)
        {
            throw std::invalid_argument("Invalid port number");
        }
    }
}

UdpProcessor::~UdpProcessor()
{
    _running = false;

    if (selfSockfd > 0)
    {
        close(selfSockfd);
    }

    if (tcpSockfd > 0)
    {
        close(tcpSockfd);
    }
}

std::optional<int> UdpProcessor::init()
{
    if ((selfSockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return std::nullopt;
    }

    // Set socket to non-blocking mode
    int flags{fcntl(selfSockfd, F_GETFL, 0)};
    if (flags == -1)
    {
        std::cerr << "Failed to get socket flags" << std::endl;
        return std::nullopt;
    }

    if (fcntl(selfSockfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        std::cerr << "Failed to set socket to non-blocking mode" << std::endl;
        return std::nullopt;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(_selfPort);

    if (bind(selfSockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr << "Bind failed" << std::endl;
        return std::nullopt;
    }

    // open and setup tcp socket:
    struct sockaddr_in tcpserverAddr{};

    if ((tcpSockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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

    if (connect(tcpSockfd, (struct sockaddr*)&tcpserverAddr, sizeof(tcpserverAddr)) < 0)
    {
        std::cerr << "TCP connection failed" << std::endl;
        return std::nullopt;
    }

    return selfSockfd;  // return udp sock
}

void UdpProcessor::run()
{
    auto sock = init();
    if (!sock)
    {
        throw std::runtime_error("Failed to create socket");
    }

    std::cout << "UDP server started on port " << _selfPort << std::endl;

    int sockfd = sock.value();

    fd_set read_fds{};
    struct timeval timeout{};

    while (_running)
    {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 500;

        int activity{select(sockfd + 1, &read_fds, nullptr, nullptr, &timeout)};
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

        if (FD_ISSET(sockfd, &read_fds))
        {
            Message receivedMessage{};

            // Receive a message
            ssize_t n{recvfrom(sockfd,
                reinterpret_cast<char*>(&receivedMessage),
                sizeof(receivedMessage),
                0,
                (struct sockaddr*)&cliaddr,
                &len)};

            if (n == sizeof(receivedMessage))
            {
                std::cout << "Received message: Type=" << static_cast<int>(receivedMessage.MessageType)
                          << ", Id=" << receivedMessage.MessageId << ", Data=" << receivedMessage.MessageData
                          << std::endl;

                _map.insert(receivedMessage); // duplicates are managed inside container

                // If MessageData equals 10, send it via TCP asynchronously
                if (receivedMessage.MessageData == 10)
                {
                    std::async(std::launch::async, &UdpProcessor::sendViaTcp, this, receivedMessage);
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

void UdpProcessor::sendViaTcp(Message message)
{
    if (send(tcpSockfd, &message, sizeof(message), 0) < 0)
    {
        std::cerr << "Failed to send message via TCP" << std::endl;
    }
}

int main()
{
    HashMap<INITIAL_CAPACITY> messageMap;
    const int ports[3] = {12345, 54321, 54312};

    UdpProcessor udpProcessor(ports[0], ports[1], ports[2], messageMap);
    udpProcessor.run();

    return 0;
}
