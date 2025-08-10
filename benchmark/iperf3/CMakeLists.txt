# CMakeLists.txt for iperf3-style TCP Benchmark

cmake_minimum_required(VERSION 3.10)

# Set the project name
project(IPerf3Benchmark)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the parent directory (assuming this is in benchmark/)
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)

# Include directories
include_directories(${PARENT_DIR}/include)

# Add the iperf3 server executable
add_executable(iperf3_server iperf3_server.cpp)

# Add the iperf3 client executable
add_executable(iperf3_client iperf3_client.cpp)

# Link against the Hohnor library
# Assuming the Hohnor library is built in the parent directory
target_link_libraries(iperf3_server 
    ${PARENT_DIR}/build/libhohnor.a  # Adjust path as needed
    pthread
)

target_link_libraries(iperf3_client 
    ${PARENT_DIR}/build/libhohnor.a  # Adjust path as needed
    pthread
)

# Compiler flags for optimization and debugging
target_compile_options(iperf3_server PRIVATE 
    -Wall -Wextra -g -O2
    -DNDEBUG  # Disable debug assertions for better performance
)

target_compile_options(iperf3_client PRIVATE 
    -Wall -Wextra -g -O2
    -DNDEBUG  # Disable debug assertions for better performance
)

# Set output directory
set_target_properties(iperf3_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(iperf3_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Optional: Create a combined target to build both
add_custom_target(benchmark_all
    DEPENDS iperf3_server iperf3_client
    COMMENT "Building all benchmark executables"
)