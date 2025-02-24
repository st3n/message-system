file(GLOB_RECURSE TCP_PROCESSOR_SOURCES src/*.cpp)
add_executable(TcpProcessor ${TCP_PROCESSOR_SOURCES})

target_include_directories(TcpProcessor
 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
PRIVATE
    ..
)

target_link_libraries(TcpProcessor PRIVATE MessagesContainer Serialization)
