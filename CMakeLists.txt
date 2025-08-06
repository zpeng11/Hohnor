cmake_minimum_required(VERSION 3.1)

project(hohnor)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#add directory, they will run the cmake file in the directory automatically
include(GNUInstallDirs)

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
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

option(ENABLE_UNIT_TEST "ENABLE_UNIT_TEST" ON) #in case cmake does not supporting FetchContent or compiler does not support C++14
if(ENABLE_UNIT_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/hohnor/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
