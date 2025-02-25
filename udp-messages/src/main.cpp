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

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << "<UDP_PORT_1> <UDP_PORT_2> <TCP_PORT> " << std::endl;
        return 1;
    }

    int udpPort1 = std::stoi(argv[1]);
    int udpPort2 = std::stoi(argv[2]);
    int tcpPort = std::stoi(argv[3]);

    HashMap<INITIAL_CAPACITY> messageMap;

    UdpServer udpProcessor1(tcpPort, udpPort1, messageMap);
    UdpServer udpProcessor2(tcpPort, udpPort2, messageMap);

    setupSignalHandler();

    std::thread udpThread1(&UdpServer::run, &udpProcessor1);

    udpProcessor2.run();

    if (udpThread1.joinable())
    {
        udpThread1.join();
    }

    return 0;
}
