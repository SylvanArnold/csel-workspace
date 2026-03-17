cmake_minimum_required(VERSION 3.28)
project("system-calls")

include(../../nanopi.cmake)
add_executable("system-calls" syscall.c)