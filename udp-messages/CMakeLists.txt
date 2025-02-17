add_executable(UdpProcessorApp src/udp_processor.cpp)

target_link_libraries(UdpProcessorApp
    PUBLIC
    MessageContainer
)

add_library(UdpProcessor STATIC src/udp_processor.cpp)

target_include_directories(UdpProcessor PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
