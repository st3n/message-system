# header only library
add_library(MessagesContainer INTERFACE)

target_include_directories(MessagesContainer INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Define the executable for testing
add_executable(ContainerTest src/container_test.cpp)

target_link_libraries(ContainerTest PRIVATE MessagesContainer)
target_include_directories(ContainerTest PRIVATE ..)
