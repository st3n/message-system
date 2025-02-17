add_executable(TcpReceiverApp src/tcp_processor.cpp)

target_link_libraries(TcpReceiverApp
    PUBLIC
    MessageContainer
)

add_library(TcpProcessor STATIC src/tcp_processor.cpp)

target_include_directories(TcpProcessor PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
