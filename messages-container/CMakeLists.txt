add_library(MessagesContainer STATIC src/lock_free_container.cpp)

target_include_directories(MessagesContainer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(ContainerTest src/container_test.cpp)
target_link_libraries(ContainerTest
    PRIVATE
    MessagesContainer
)
