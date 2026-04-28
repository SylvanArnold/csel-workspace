cmake_minimum_required(VERSION 3.28)
project(silly_led_control)

set(CMAKE_SYSTEM_NAME Linux)

# Which compilers to use for C and C++ (cross-compilation)
set(CMAKE_C_COMPILER /buildroot/output/host/bin/aarch64-linux-gcc)
set(CMAKE_CXX_COMPILER /buildroot/output/host/bin/aarch64-linux-g++)

# Set the path to sysroot
set(CMAKE_FIND_ROOT_PATH /buildroot/output/host)


set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Flags for develompent and debugging builds
set(CMAKE_C_FLAGS "-Wall -Wextra -g -O0 -MD -std=gnu17")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a53 -funwind-tables")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -g -O0 -MD -std=gnu++17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -funwind-tables")

add_executable(silly_led_control silly_led_control.c)