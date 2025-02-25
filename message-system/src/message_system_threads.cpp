#include <messages-container/blocking/hash_map.hpp>
#include <tcp-messages/tcp_processor.hpp>
#include <udp-messages/udp_processor.hpp>
#include <common/signal_handler.hpp>

#include <iostream>
#include <memory>
#include <thread>
#include <csignal>



void runUdpProcessor(int16_t port, int16_t tcpPort, HashMap<INITIAL_CAPACITY>& map)
{
    UdpServer udpServer(tcpPort, port, map);

    udpServer.run();
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <UDP Port 1> <UDP Port 2> <TCP Port>\n";
        return 1;
    }

    int16_t udpPort1 = std::stoi(argv[1]);
    int16_t udpPort2 = std::stoi(argv[2]);
    int16_t tcpPort = std::stoi(argv[3]);

    setupSignalHandler();

    HashMap<INITIAL_CAPACITY> messageMap;

    std::thread udpThread1(runUdpProcessor, udpPort1, tcpPort, std::ref(messageMap));
    std::thread udpThread2(runUdpProcessor, udpPort2, tcpPort, std::ref(messageMap));

    TcpServer tcpServer(tcpPort);
    tcpServer.run();

    udpThread1.join();
    udpThread2.join();

    return 0;
}
