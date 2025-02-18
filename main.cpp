#include "message-container/container.hpp"
#include "tcp-messages/tcp_processor.hpp"
#include "udp-messages/udp_processor.hpp"

#include <iostream>

int main()
{
    MessageContainer container;

    UdpProcessor receiver1(UDP_PORT_1, container);
    UdpProcessor receiver2(UDP_PORT_2, container);

    receiver1.start();
    receiver2.start();

    while (true)
    {
        sleep(1);
    }

    return 0;
}
