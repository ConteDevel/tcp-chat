cmake_minimum_required(VERSION 2.8)

project(chat)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pedantic")
add_executable(
    chat_server
    "server.c"
    "common.h"
    "common.c"
    "message.h"
    "message.c"
)
add_executable(
    chat_client
    "client.c"
    "common.h"
    "common.c"
    "message.h"
    "message.c"
)
