#include "message.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sys/socket.h>
#include <thread>
#include <vector>

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
        dist_data(rng),  // MessageId
        dist_data(rng)  // MessageData
    };
}


void send_message_and_log(const std::string& ip, int port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    for (size_t i = 0; i < 100; i++)
    {
        Message msg = generate_random_message();
        {
            std::lock_guard<std::mutex> lock(file_mutex);
            std::ofstream log_file("messages_system.log", std::ios::app);
            log_file << "Size: " << msg.MessageSize << " Type: " << msg.MessageType << " ID: " << msg.MessageId
                     << " Data: " << msg.MessageData << std::endl;
        }

        sendto(sock, &msg, sizeof(msg), 0, (sockaddr*)&addr, sizeof(addr));
    }

    close(sock);
}

}  // namespace

int main()
{
    const int num_threads = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(send_message_and_log, "127.0.0.1", (rand() % 2 == 0) ? UDP_PORT_1 : UDP_PORT_2);
    }

    for (auto& t : threads)
    {
        t.join();
    }

    return 0;
}
