add_library(netring
    netring/net.cc
    netring/uring.cc
    netring/uf.cc
)
target_include_directories(netring PUBLIC .)

target_link_libraries(netring fmt)
target_link_libraries(netring fallible)
target_link_libraries(netring sure)
target_link_libraries(netring five)

target_include_directories(netring PUBLIC /home/fed/src/liburing/src/include)
target_link_libraries(netring /home/fed/src/liburing/src/liburing.a)
