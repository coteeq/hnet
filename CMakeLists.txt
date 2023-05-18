cmake_minimum_required (VERSION 3.20)
project(hnet CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_compile_options(-Wall -Wextra -g -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer)
# add_compile_options(-Wpedantic)
add_compile_options(-Werror)

set(SRV_SRCS
    src/srv2.cc
)

set(CLI_SRCS
    src/cli.cc
)

add_subdirectory(third)
add_subdirectory(wheels)
add_subdirectory(fallible/fallible)
add_subdirectory(five)
add_subdirectory(ring2)
add_subdirectory(sure)
add_subdirectory(tinyfibers)

# target_compile_options(net PRIVATE -Wno-pedantic)

add_executable(srv ${SRV_SRCS})
add_executable(cli ${CLI_SRCS})
add_executable(cli_udp src/infallible_udp.cc)
target_link_libraries(srv
    ring2
    five
    sure
    tinyfibers
)

target_link_libraries(cli
    ring2
    five
    tinyfibers
)

target_link_libraries(cli_udp
    ring2
    five
    tinyfibers
)
