cmake_minimum_required(VERSION 3.11)

project(Hohnor)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)



include_directories(${PROJECT_SOURCE_DIR}/src/common)

#add directory, they will run the cmake file in the directory automatically 
add_subdirectory(src/thread)
add_subdirectory(src/time)
add_subdirectory(src/log)
add_subdirectory(src/file)
add_subdirectory(src/process)
add_subdirectory(src/net)
add_subdirectory(src/io)
add_subdirectory(src/core)
add_subdirectory(example)
add_subdirectory(tests)

list(APPEND libs THREAD)
list(APPEND libs TIME)
list(APPEND libs LOG)
list(APPEND libs FILE)
list(APPEND libs PROCESS)
list(APPEND libs NET)
list(APPEND libs IO)
list(APPEND libs CORE)

