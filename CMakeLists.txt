cmake_minimum_required(VERSION 3.10)

project(VEDA_Chat C)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/src/membermanager.c
)

add_executable(chat_server ${SOURCES})
