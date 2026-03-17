set(CMAKE_SYSTEM_NAME Linux)

find_program(CMAKE_C_COMPILER aarch64-linux-gcc)
find_program(CMAKE_CXX_COMPILER aarch64-linux-g++)
set(CMAKE_FIND_ROOT_PATH /buildroot/output/host)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_FLAGS "-Wall -Wextra -g -O0 -MD -std=gnu17")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a53 -funwind-tables")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -g -O0 -MD -std=gnu++17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -funwind-tables")
