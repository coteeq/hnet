cmake_minimum_required (VERSION 3.20)
project(hnet CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_compile_options(-Wall -Wextra -g -fno-omit-frame-pointer)
# add_compile_options(-Wpedantic)
add_compile_options(-Werror)

set(COMMON_SRCS
    # src/reh.cc
)

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
add_subdirectory(netring)
add_subdirectory(ring2)
add_subdirectory(sure)
add_subdirectory(tinyfibers)

# target_compile_options(net PRIVATE -Wno-pedantic)

add_executable(srv ${SRV_SRCS} ${COMMON_SRCS})
add_executable(cli ${CLI_SRCS} ${COMMON_SRCS})
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
