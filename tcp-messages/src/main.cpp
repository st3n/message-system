#include "tcp-messages/tcp_processor.hpp"

#include <iostream>


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <PORT>" << std::endl;
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(std::atoi(argv[1]));
    TcpServer server(port);
    server.run();

    return 0;
}
