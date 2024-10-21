enable_testing()

add_executable(
    common_tests 
    binaryheaptest.cpp 
    circularbuffertest.cpp)

target_link_libraries(
    common_tests
    Hohnor
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(common_tests)