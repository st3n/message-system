cmake_minimum_required(VERSION 3.10)
project(NetworkSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread")

# Library for message container
add_subdirectory(message-container)

# UDP Sender-Receiver Application
add_subdirectory(udp-messages)

# TCP Receiver Application
add_subdirectory(tcp-messages)

# Common Application (UDP receive/send, TCP receive + message container)
add_executable(messageProcessor main.cpp)

target_link_libraries(messageProcessor
    PRIVATE
    UdpProcessor
    TcpProcessor
)
