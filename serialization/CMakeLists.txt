cmake_minimum_required(VERSION 3.10)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/serializer.cpp)
set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/serializer.hpp)

add_library(Serialization STATIC ${SOURCES})

target_sources(Serialization PUBLIC ${HEADERS})

target_include_directories(Serialization
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../
)
