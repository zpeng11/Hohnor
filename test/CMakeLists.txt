cmake_minimum_required(VERSION 3.14)


# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

add_subdirectory(common)

add_executable(LittleTest Test.cpp)
add_executable(LittleTestClient TestClient.cpp)
target_link_libraries(LittleTest PUBLIC Hohnor)
target_link_libraries(LittleTestClient PUBLIC Hohnor)