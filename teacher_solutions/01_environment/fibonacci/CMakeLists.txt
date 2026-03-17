cmake_minimum_required(VERSION 3.28)
project(fibonacci)

include(../../nanopi.cmake)
add_executable(fibonacci fibonacci.c)