cmake_minimum_required(VERSION 3.50)
project(simple_reverse_proxy VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)


file(GLOB SRC_ALL *.c)

add_library(src_all ${SRC_ALL})

add_executable(srp main.c)
target_link_libraries(srp src_all)
