enable_testing()

add_executable(
    thread_tests 
    threadtest.cpp
    threadpooltest.cpp
    )

target_link_libraries(
    thread_tests
    Hohnor
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(thread_tests)