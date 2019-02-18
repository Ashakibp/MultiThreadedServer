cmake_minimum_required(VERSION 3.12)
project(classFiles C)

set(CMAKE_C_STANDARD 11)

add_executable(classFiles
        client.c
        server.c)
