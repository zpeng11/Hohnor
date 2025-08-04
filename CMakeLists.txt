cmake_minimum_required(VERSION 3.1)

project(Hohnor)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
#add directory, they will run the cmake file in the directory automatically 
add_subdirectory(hohnor)
# add_subdirectory(example)



list(APPEND libs THREAD)
list(APPEND libs TIME)
list(APPEND libs LOG)
list(APPEND libs FILE)
list(APPEND libs PROCESS)
list(APPEND libs NET)
list(APPEND libs IO)
list(APPEND libs CORE)

option(ENABLE_UNIT_TEST "ENABLE_UNIT_TEST" OFF) #in case cmake does not supporting FetchContent or compiler does not support C++14
if(ENABLE_UNIT_TEST)
    add_subdirectory(test)
endif()
