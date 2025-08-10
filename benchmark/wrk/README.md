# Hohnor wrk-compatible TCP Benchmark

This directory contains a high-performance HTTP server and client implementation designed to work with the `wrk` HTTP benchmarking tool. The implementation uses the Hohnor library's [`EventLoop`](../../include/hohnor/core/EventLoop.h:33), [`TCPAcceptor`](../../include/hohnor/net/TCPAcceptor.h:18), [`TCPConnector`](../../include/hohnor/net/TCPConnector.h:14), and [`TCPConnection`](../../include/hohnor/net/TCPConnection.h:28) classes to provide efficient TCP handling.

## Overview

The benchmark consists of two main components:

1. **wrk_server**: A high-performance HTTP server that can handle thousands of concurrent connections
2. **wrk_client**: A load testing client that can simulate HTTP traffic similar to wrk

## Files

- [`wrk_server.cpp`](wrk_server.cpp:1) - HTTP server implementation
- [`wrk_client.cpp`](wrk_client.cpp:1) - HTTP client implementation  
- [`CMakeLists.txt`](CMakeLists.txt:1) - Build configuration
- [`test_wrk.sh.in`](test_wrk.sh.in:1) - Test script template
- [`README.md`](README.md:1) - This documentation

## Building

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+)
- Built Hohnor library (libhohnor.a)

### Build Steps

1. **Build the main Hohnor library first** (from project root):
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make -j$(nproc)
   cd ..
   ```

2. **Build the wrk benchmark**:
   ```bash
   cd benchmark/wrk
   mkdir -p build
   cd build
   cmake ..
   make wrk_benchmark_all
   ```

   This will create:
   - `wrk_server` - The HTTP server executable
   - `wrk_client` - The HTTP client executable
   - `test_wrk.sh` - Automated test script

## Usage

### HTTP Server

Start the server:
```bash
./wrk_server [options]
```

**Options:**
- `-p, --port <port>` - Server port to listen on (default: 8080)
- `-h, --help` - Show help message

**Example:**
```bash
./wrk_server -p 8080
```

The server will display:
```
====================================================
wrk-compatible HTTP Server started
Listening on: http://localhost:8080
Ready for wrk benchmarking
====================================================
Example wrk command:
  wrk -t12 -c400 -d30s http://localhost:8080/
====================================================
```

### HTTP Client

Run the client:
```bash
./wrk_client [options]
```

**Options:**
- `-h, --host <host>` - Server hostname (default: localhost)
- `-p, --port <port>` - Server port (default: 8080)
- `-c, --connections <n>` - Number of connections (default: 10)
- `-t, --time <sec>` - Test duration in seconds (default: 10)
- `--help` - Show help message

**Example:**
```bash
./wrk_client -h localhost -p 8080 -c 100 -t 30
```

### Automated Testing

Use the provided test script:
```bash
./test_wrk.sh [options]
```

**Options:**
- `-p, --port <port>` - Server port (default: 8080)
- `-c, --connections <num>` - Number of connections (default: 100)
- `-t, --time <seconds>` - Test duration (default: 10)
- `-h, --help` - Show help

**Example:**
```bash
./test_wrk.sh -p 8080 -c 200 -t 30
```

## Testing with wrk

If you have `wrk` installed, you can test the server directly:

1. **Start the server**:
   ```bash
   ./wrk_server -p 8080
   ```

2. **Run wrk benchmark**:
   ```bash
   # Basic test
   wrk -t12 -c400 -d30s http://localhost:8080/
   
   # High load test
   wrk -t16 -c1000 -d60s --timeout 10s http://localhost:8080/
   
   # Custom script test
   wrk -t8 -c200 -d30s -s script.lua http://localhost:8080/
   ```

## Performance Tuning

### Server Optimizations

The server includes several performance optimizations:

1. **TCP_NODELAY**: Disables Nagle's algorithm for lower latency
2. **SO_REUSEADDR**: Allows rapid server restarts
3. **SO_REUSEPORT**: Enables port sharing for multi-process setups
4. **Keep-alive connections**: Reduces connection overhead
5. **Efficient buffer management**: Uses Hohnor's [`Buffer`](../../include/hohnor/common/Buffer.h:23) class

### System Tuning

For high-performance testing, consider these system tunings:

```bash
# Increase file descriptor limits
ulimit -n 65536

# Tune TCP settings
echo 'net.core.somaxconn = 65536' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_max_syn_backlog = 65536' >> /etc/sysctl.conf
echo 'net.core.netdev_max_backlog = 5000' >> /etc/sysctl.conf
sysctl -p
```

## Architecture

### Server Architecture

The server uses the following Hohnor components:

- **[`EventLoop`](../../include/hohnor/core/EventLoop.h:33)**: Main event loop for handling I/O events
- **[`TCPAcceptor`](../../include/hohnor/net/TCPAcceptor.h:18)**: Accepts incoming TCP connections
- **[`TCPConnection`](../../include/hohnor/net/TCPConnection.h:28)**: Manages individual client connections
- **[`Buffer`](../../include/hohnor/common/Buffer.h:23)**: Efficient buffer management for I/O operations

### Client Architecture

The client uses:

- **[`EventLoop`](../../include/hohnor/core/EventLoop.h:33)**: Event loop for managing multiple connections
- **[`TCPConnector`](../../include/hohnor/net/TCPConnector.h:14)**: Establishes connections to the server
- **[`TCPConnection`](../../include/hohnor/net/TCPConnection.h:28)**: Handles HTTP request/response cycles

### Connection Flow

1. **Server startup**: Creates [`TCPAcceptor`](../../include/hohnor/net/TCPAcceptor.h:18) and starts listening
2. **Client connection**: [`TCPConnector`](../../include/hohnor/net/TCPConnector.h:14) establishes connection
3. **HTTP exchange**: Client sends requests, server responds
4. **Keep-alive**: Connections are reused for multiple requests
5. **Graceful shutdown**: Proper cleanup on termination

## Benchmarking Results

### Expected Performance

On a modern system, you should expect:

- **Requests/sec**: 50,000 - 200,000+ (depending on hardware)
- **Latency**: Sub-millisecond for simple responses
- **Concurrent connections**: 10,000+ simultaneous connections
- **Memory usage**: Low memory footprint per connection

### Sample Output

**Server output:**
```
====================================================
wrk-compatible HTTP Server started
Listening on: http://localhost:8080
Ready for wrk benchmarking
====================================================
[5.0s] Requests: 125000 (25000 req/s), Connections: 400, RX: 15.2 Mbps, TX: 45.8 Mbps
[10.0s] Requests: 250000 (25000 req/s), Connections: 400, RX: 15.2 Mbps, TX: 45.8 Mbps
```

**wrk output:**
```
Running 30s test @ http://localhost:8080/
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.15ms    1.23ms  15.67ms   89.23%
    Req/Sec     2.08k   234.56    3.12k    78.45%
  750000 requests in 30.00s, 125.67MB read
Requests/sec:  25000.12
Transfer/sec:   4.19MB
```

## Troubleshooting

### Common Issues

1. **Port already in use**:
   ```bash
   # Find process using the port
   lsof -i :8080
   # Kill the process
   kill -9 <PID>
   ```

2. **Too many open files**:
   ```bash
   # Increase file descriptor limit
   ulimit -n 65536
   ```

3. **Connection refused**:
   - Ensure server is running
   - Check firewall settings
   - Verify port number

4. **Low performance**:
   - Check system resource usage (CPU, memory)
   - Tune TCP parameters
   - Increase file descriptor limits
   - Use multiple server processes

### Debug Mode

For debugging, build without `-DNDEBUG`:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make wrk_benchmark_all
```

## Integration with CI/CD

### Automated Testing

Add to your CI pipeline:

```yaml
# Example GitHub Actions
- name: Build wrk benchmark
  run: |
    cd benchmark/wrk
    mkdir build && cd build
    cmake ..
    make wrk_benchmark_all

- name: Run benchmark test
  run: |
    cd benchmark/wrk/build
    timeout 60s ./test_wrk.sh -c 100 -t 10
```

### Performance Regression Testing

```bash
#!/bin/bash
# performance_test.sh
BASELINE_RPS=20000
CURRENT_RPS=$(./test_wrk.sh -c 100 -t 10 | grep "req/s" | awk '{print $2}')

if [ "$CURRENT_RPS" -lt "$BASELINE_RPS" ]; then
    echo "Performance regression detected: $CURRENT_RPS < $BASELINE_RPS"
    exit 1
fi
```

## Contributing

When modifying the benchmark:

1. Maintain compatibility with standard HTTP/1.1
2. Preserve keep-alive connection support
3. Update documentation for new features
4. Add appropriate error handling
5. Test with various load patterns

## See Also

- [Hohnor EventLoop Documentation](../../include/hohnor/core/EventLoop.h:33)
- [TCP Connection Management](../../include/hohnor/net/TCPConnection.h:28)
- [iperf3 Benchmark](../iperf3/README.md) - TCP throughput testing
- [wrk HTTP benchmarking tool](https://github.com/wg/wrk)