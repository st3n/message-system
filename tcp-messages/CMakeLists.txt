add_executable(TcpProcessor src/tcp_processor.cpp)

target_include_directories(TcpProcessor
 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
PRIVATE
    ..
)

target_link_libraries(TcpProcessor PRIVATE MessagesContainer)
