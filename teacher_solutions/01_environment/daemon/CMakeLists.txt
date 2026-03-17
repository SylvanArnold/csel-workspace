cmake_minimum_required(VERSION 3.28)
project(daemon)

include(../../nanopi.cmake)
add_executable(daemon daemon.c)