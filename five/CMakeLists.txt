set(SRCS
    five/type_name.cc
    five/xrange.cc
)

add_library(five ${SRCS})
target_include_directories(five PUBLIC .)

target_link_libraries(five wheels)
target_link_libraries(five tomlplusplus_tomlplusplus)
add_subdirectory(five/ut)
