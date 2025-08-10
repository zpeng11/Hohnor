# CMakeLists.txt for keyboard example

cmake_minimum_required(VERSION 3.10)

# Set the project name
project(SnakeGame)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the parent directory (assuming this is in example/keyboard)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)

# Include directories
include_directories(${PARENT_DIR}/include)

# Add the executable
add_executable(${PROJECT_NAME} snake_game.cpp)

# Link against the Hohnor library
# Assuming the Hohnor library is built in the parent directory
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    ${PARENT_DIR}/build/libhohnor.a  # Adjust path as needed
    pthread
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -g)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)



include(ExternalProject)
include(FindPackageHandleStandardArgs)

# Set installation path for the external project
set(NCURSES_INSTALL_DIR ${CMAKE_BINARY_DIR}/ncurses_install)

ExternalProject_Add(
  ncurses
  URL https://invisible-mirror.net/archives/ncurses/ncurses-6.4.tar.gz
  PREFIX ${CMAKE_BINARY_DIR}/ncurses
  CONFIGURE_COMMAND ./configure --prefix=${NCURSES_INSTALL_DIR} --with-shared --without-debug --without-ada
  BUILD_COMMAND make -j
  INSTALL_COMMAND make install
  BUILD_IN_SOURCE 1
)

# Make sure your app builds after ncurses is built
add_dependencies(${PROJECT_NAME} ncurses)

# Add ncurses include path
target_include_directories(${PROJECT_NAME} PRIVATE ${NCURSES_INSTALL_DIR}/include)

# Add ncurses library path
target_link_directories(${PROJECT_NAME} PRIVATE ${NCURSES_INSTALL_DIR}/lib)

# Link to ncurses
target_link_libraries(${PROJECT_NAME} PRIVATE ${NCURSES_INSTALL_DIR}/lib/libncurses.so)