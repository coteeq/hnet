enable_testing()

add_executable(
  bit_ptr_test
  bit_ptr.cc
)
target_link_libraries(
  bit_ptr_test
  GTest::gtest_main
  five
)

include(GoogleTest)
gtest_discover_tests(bit_ptr_test)
