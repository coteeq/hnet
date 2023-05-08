add_library(ring2
    ring2/ring.h
    ring2/ring.cc
    ring2/addr.h
    ring2/addr.cc
    ring2/poller.h
    ring2/poller.cc
    ring2/submitter.h
    ring2/submitter.cc
    ring2/sock.h
    ring2/sock.cc
)
target_include_directories(ring2 PUBLIC .)

target_link_libraries(ring2 wheels)
target_link_libraries(ring2 fmt)
target_link_libraries(ring2 fallible)
target_link_libraries(ring2 five)
target_link_libraries(ring2 function2)
target_link_libraries(ring2 tinyfibers)

target_include_directories(ring2 PUBLIC /home/coteeq/src/liburing/src/include)
target_link_libraries(ring2 /home/coteeq/src/liburing/src/liburing.a)
