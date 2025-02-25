#include <arpa/inet.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sys/socket.h>
#include <thread>
#include <vector>

#include <cstring>

#include <message.hpp>
#include <serializer.hpp>

namespace
{

std::mutex file_mutex;
thread_local std::mt19937_64 rng(std::random_device{}());

std::uniform_int_distribution<uint64_t> key_dist(0, UINT64_MAX);
std::uniform_int_distribution<uint16_t> dist_size(1, 1024);
std::uniform_int_distribution<uint8_t> dist_type(0, 255);
std::uniform_int_distribution<uint64_t> dist_data(0, UINT64_MAX);

Message generate_random_message()
{
    return Message{
        dist_size(rng),  // MessageSize
        dist_type(rng),  // MessageType
        key_dist(rng),  // MessageId
        dist_data(rng)  // MessageData
    };
}

void send_message_and_log(const std::string& ip, int port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return;

    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
    {
        std::cerr << "Connect failed: " << strerror(errno) << std::endl;
        close(sock);
        return;
    }

    for (size_t i = 0; i < 100; i++)
    {
        Message msg = generate_random_message();
        sendMessage(sock, msg);
    }

    for (size_t i = 0; i < 10; i++)
    {
        Message msg = generate_random_message();
        msg.MessageData = 10;
        {
            std::lock_guard<std::mutex> lock(file_mutex);
            std::ofstream log_file("messages_system.log", std::ios::app);
            log_file << "Size: " << msg.MessageSize << " Type: " << msg.MessageType << " ID: " << msg.MessageId
                     << " Data: " << msg.MessageData << std::endl;
        }

        // for verification
        sendMessage(sock, msg);
    }

    close(sock);
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << "<UDP_PORT_1> <UDP_PORT_2> " << std::endl;
        return 1;
    }

    uint16_t port1 = static_cast<uint16_t>(std::atoi(argv[1]));
    uint16_t port2 = static_cast<uint16_t>(std::atoi(argv[1]));

    const int num_threads = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(send_message_and_log, "127.0.0.1", (rand() % 2 == 0) ? port1 : port2);
    }

    for (auto& t : threads)
    {
        t.join();
    }

    return 0;
}
