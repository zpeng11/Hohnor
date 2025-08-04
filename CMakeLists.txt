cmake_minimum_required(VERSION 3.1)

project(Hohnor)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
#add directory, they will run the cmake file in the directory automatically 



file(GLOB SOURCES
    "src/thread/*.cpp"
    "src/time/*.cpp"
    "src/log/*.cpp"
    "src/file/*.cpp"
    "src/process/*.cpp"
    "src/net/*.cpp"
    "src/io/*.cpp"
    "src/core/*.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

option(ENABLE_UNIT_TEST "ENABLE_UNIT_TEST" OFF) #in case cmake does not supporting FetchContent or compiler does not support C++14
if(ENABLE_UNIT_TEST)
    add_subdirectory(test)
endif()
