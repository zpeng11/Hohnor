# CMakeLists.txt for keyboard example

cmake_minimum_required(VERSION 3.10)

# Set the project name
project(KeyboardExample)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the parent directory (assuming this is in example/keyboard)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)

# Include directories
include_directories(${PARENT_DIR}/include)

# Add the executable
add_executable(keyboard_example keyboard_example.cpp)

# Link against the Hohnor library
# Assuming the Hohnor library is built in the parent directory
target_link_libraries(keyboard_example 
    ${PARENT_DIR}/build/libhohnor.a  # Adjust path as needed
    pthread
)

# Compiler flags
target_compile_options(keyboard_example PRIVATE -Wall -Wextra -g)

# Set output directory
set_target_properties(keyboard_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)