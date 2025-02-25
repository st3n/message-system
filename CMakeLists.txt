cmake_minimum_required(VERSION 3.10)
project(NetworkSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread")

# Set build types
# set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")

# Set flags for Debug and Release builds
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=thread -fno-omit-frame-pointer")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Iterface for message container
add_subdirectory(messages-container)

# UDP Sender-Receiver Application, Lib
add_subdirectory(udp-messages)

# TCP Receiver Application, Lib
add_subdirectory(tcp-messages)

# serialization
add_subdirectory(serialization)

# common
add_subdirectory(common)

# Common Application (UDP receive/send, TCP receive + message container)
add_subdirectory(message-system)
