cmake_minimum_required(VERSION 3.10)

project(NetworkProcessorApp)

add_executable(NetworkProcessorApp src/message_system_threads.cpp)

target_link_libraries(NetworkProcessorApp
    PRIVATE
        Common
        MessagesContainer
        UdpProcessorLib
        TcpProcessorLib
)

# Include directories if needed
target_include_directories(NetworkProcessorApp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
