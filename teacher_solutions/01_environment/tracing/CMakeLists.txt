cmake_minimum_required(VERSION 3.28)
project(tracing)

include(../../nanopi.cmake)

add_executable(fibonacci fibonacci.c)
add_executable(fibonacci-debug fibonacci.c)

target_compile_definitions(fibonacci-debug PRIVATE DEBUG)