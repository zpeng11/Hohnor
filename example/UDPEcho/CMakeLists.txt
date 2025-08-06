# CMakeLists.txt for UDP Echo example

cmake_minimum_required(VERSION 3.10)

# Set the project name
project(UDPEchoExample)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the parent directory (assuming this is in example/UDPEcho)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)

# Include directories
include_directories(${PARENT_DIR}/include)

# Add the UDP echo server executable
add_executable(udp_echo_server echo_server.cpp)

# Add the UDP echo client executable
add_executable(udp_echo_client echo_client.cpp)

# Link against the Hohnor library
# Assuming the Hohnor library is built in the parent directory
target_link_libraries(udp_echo_server 
    ${PARENT_DIR}/build/libhohnor.a  # Adjust path as needed
    pthread
)

target_link_libraries(udp_echo_client 
    ${PARENT_DIR}/build/libhohnor.a  # Adjust path as needed
    pthread
)

# Compiler flags
target_compile_options(udp_echo_server PRIVATE -Wall -Wextra -g)
target_compile_options(udp_echo_client PRIVATE -Wall -Wextra -g)

# Set output directory
set_target_properties(udp_echo_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(udp_echo_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)