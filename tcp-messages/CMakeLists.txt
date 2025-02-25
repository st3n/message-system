add_library(TcpProcessorLib STATIC src/tcp_processor.cpp)

add_executable(TcpProcessor src/tcp_processor.cpp src/main.cpp)

target_include_directories(TcpProcessorLib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ..
)

target_include_directories(TcpProcessor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ..
)

target_link_libraries(TcpProcessorLib PUBLIC MessagesContainer Serialization Common)
target_link_libraries(TcpProcessor PRIVATE MessagesContainer Serialization Common)
