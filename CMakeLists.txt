cmake_minimum_required(VERSION 3.10)

project(VEDA_Chat C)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/membermanager.c
    ${CMAKE_SOURCE_DIR}/src/parentprocess.c
    ${CMAKE_SOURCE_DIR}/src/childprocess.c
    ${CMAKE_SOURCE_DIR}/src/daemonize.c
    ${CMAKE_SOURCE_DIR}/main.c
)

add_executable(chat_server ${SOURCES})
