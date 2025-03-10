cmake_minimum_required(VERSION 3.30)
project(mellow_api C)

set(CMAKE_C_STANDARD 17)

add_executable(mellow_api main.c
        src/query.c)
add_executable(mellow_api main.c
        src/testing.c
        src/testing.c)
