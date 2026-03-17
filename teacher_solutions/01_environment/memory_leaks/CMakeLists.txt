cmake_minimum_required(VERSION 3.28)
project(memory-leaks)

include(../../nanopi.cmake)
add_executable(memory-leaks mem_leaks.c)