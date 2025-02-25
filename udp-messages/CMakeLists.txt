add_library(UdpProcessorLib STATIC src/udp_processor.cpp)

add_executable(UdpProcessor src/udp_processor.cpp src/main.cpp)

target_include_directories(UdpProcessorLib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ..
)

target_include_directories(UdpProcessor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ..
)

target_link_libraries(UdpProcessorLib PUBLIC MessagesContainer Serialization Common)
target_link_libraries(UdpProcessor PRIVATE MessagesContainer Serialization Common)
