cmake_minimum_required(VERSION 3.28)
project(core-dumps)

include(../../nanopi.cmake)
add_executable(core-dumps core_dumps.c)