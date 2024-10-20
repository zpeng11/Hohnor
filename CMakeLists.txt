cmake_minimum_required(VERSION 3.1)

project(Hohnor)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)



include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hohnor)

#add directory, they will run the cmake file in the directory automatically 
add_subdirectory(hohnor/thread)
add_subdirectory(hohnor/time)
add_subdirectory(hohnor/log)
add_subdirectory(hohnor/file)
add_subdirectory(hohnor/process)
add_subdirectory(hohnor/net)
add_subdirectory(hohnor/io)
add_subdirectory(hohnor/core)
add_subdirectory(example)

list(APPEND libs THREAD)
list(APPEND libs TIME)
list(APPEND libs LOG)
list(APPEND libs FILE)
list(APPEND libs PROCESS)
list(APPEND libs NET)
list(APPEND libs IO)
list(APPEND libs CORE)


add_executable(Test test/Test.cpp)
add_executable(TestClient test/TestClient.cpp)
target_link_libraries(Test PUBLIC ${libs})
target_link_libraries(TestClient PUBLIC ${libs})