cmake_minimum_required(VERSION 3.14)

project(Hohnor)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#Add cmake dir 
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#add directory, they will run the cmake file in the directory automatically 
add_subdirectory(src)
add_subdirectory(tests)


