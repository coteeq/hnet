enable_testing()

add_executable(
  five_ut
  bit_ptr.cc
  time.cc
  xrange.cc
)

target_link_libraries(
  five_ut
  GTest::gtest_main
  five
)

include(GoogleTest)
gtest_discover_tests(five_ut)
