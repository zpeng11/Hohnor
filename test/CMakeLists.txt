include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Find all test files
file(GLOB_RECURSE TEST_SOURCES "time/*.cpp" "thread/*.cpp" "process/*.cpp" "file/*.cpp")

# Add the main test executable
add_executable(runTests TestMain.cpp ${TEST_SOURCES})

# Link against the project library and gtest
target_link_libraries(runTests PRIVATE ${PROJECT_NAME} gtest_main)

# Add test to CTest
include(GoogleTest)
gtest_discover_tests(runTests)