#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

struct Message
{
    uint16_t MessageSize;
    uint8_t MessageType;
    uint64_t MessageId;
    uint64_t MessageData;
} __attribute__((packed));

pid_t start_process(const std::string& cmd)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(1);
    }

    return pid;
}

void send_message(const std::string& ip, int port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    Message msg;
    msg.MessageSize = sizeof(Message);
    msg.MessageType = 1;
    msg.MessageId = rand();
    msg.MessageData = rand() % 20;

    sendto(sock, &msg, sizeof(msg), 0, (sockaddr*)&addr, sizeof(addr));
    close(sock);
}

void terminate_process(pid_t pid)
{
    if (pid > 0)
        kill(pid, SIGTERM);

    waitpid(pid, nullptr, 0);
}

int main()
{
    std::string udp_cmd = "./udp-messages/UdpProcessor 50001 50002 50003";
    std::string tcp_cmd = "./tcp-messages/TcpProcessor 50001";

    pid_t udp_pid = start_process(udp_cmd);
    pid_t tcp_pid = start_process(tcp_cmd);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 0; i < 1000; ++i)
    {
        send_message("127.0.0.1", (rand() % 2 == 0) ? 50002 : 50003);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    terminate_process(udp_pid);
    terminate_process(tcp_pid);

    return 0;
}
