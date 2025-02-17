add_library(MessageContainer STATIC src/message_container.cpp)

target_include_directories(MessageContainer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
