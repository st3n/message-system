add_executable(UdpProcessor src/udp_processor.cpp)

target_include_directories(UdpProcessor
 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
PRIVATE
    ..
)

target_link_libraries(UdpProcessor PRIVATE MessagesContainer)
