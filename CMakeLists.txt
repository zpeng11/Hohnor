cmake_minimum_required(VERSION 3.1)

project(Hohnor)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)



include_directories(${PROJECT_SOURCE_DIR}/src)

#add directory, they will run the cmake file in the directory automatically 
add_subdirectory(src/thread)
add_subdirectory(src/time)
add_subdirectory(src/log)
add_subdirectory(src/file)
add_subdirectory(src/process)

list(APPEND libs THREAD)
list(APPEND libs TIME)
list(APPEND libs LOG)
list(APPEND libs FILE)
list(APPEND libs PROCESS)


add_executable(Test test/Test.cpp)
target_link_libraries(Test PUBLIC ${libs})