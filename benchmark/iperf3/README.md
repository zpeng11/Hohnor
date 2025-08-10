# iperf3-style TCP Benchmark for Hohnor

This directory contains iperf3-style TCP benchmark tools built using the Hohnor networking library. The benchmark consists of a server and client that can measure TCP throughput performance.

## Overview

The benchmark implements the core functionality of iperf3:
- **Server**: Receives data and measures throughput
- **Client**: Sends data and measures throughput
- Real-time statistics reporting
- Configurable test duration and buffer sizes
- Compatible command-line interface similar to iperf3

## Files

- `iperf3_server.cpp` - TCP server implementation
- `iperf3_client.cpp` - TCP client implementation  
- `CMakeLists.txt` - Build configuration
- `README.md` - This documentation

## Building

### Prerequisites

1. **Build the Hohnor library first**:
   ```bash
   cd /path/to/Hohnor
   mkdir -p build
   cd build
   cmake ..
   make -j$(nproc)
   ```

2. **Build the benchmark tools**:
   ```bash
   cd /path/to/Hohnor/benchmark
   mkdir -p build
   cd build
   cmake ..
   make -j$(nproc)
   ```

This will create two executables:
- `iperf3_server` - The benchmark server
- `iperf3_client` - The benchmark client

## Usage

### Server Mode

Start the server to listen for incoming connections:

```bash
# Basic server (listens on port 5201, unlimited duration)
./iperf3_server -s

# Server with custom port
./iperf3_server -s -p 8080

# Server with test duration limit
./iperf3_server -s -t 30
```

**Server Options:**
- `-s, --server` - Run in server mode
- `-p, --port <port>` - Server port to listen on (default: 5201)
- `-t, --time <sec>` - Time in seconds to run (default: unlimited)
- `-h, --help` - Show help message

### Client Mode

Connect to a server and run throughput test:

```bash
# Basic client test (10 seconds to localhost:5201)
./iperf3_client -c 127.0.0.1

# Client with custom server and port
./iperf3_client -c 192.168.1.100 -p 8080

# Client with custom test duration
./iperf3_client -c 127.0.0.1 -t 30

# Client with custom buffer size
./iperf3_client -c 127.0.0.1 -l 256K
```

**Client Options:**
- `-c, --client <host>` - Run in client mode, connecting to <host>
- `-p, --port <port>` - Server port to connect to (default: 5201)
- `-t, --time <sec>` - Time in seconds to transmit (default: 10)
- `-l, --length <len>` - Length of buffer to read or write (default: 128K)
- `-P, --parallel <n>` - Number of parallel client streams (default: 1, not yet implemented)
- `-h, --help` - Show help message

## Example Test Session

### Terminal 1 (Server):
```bash
$ ./iperf3_server -s
-----------------------------------------------------------
Server listening on 5201
Test duration: unlimited (until Ctrl+C)
TCP window size: 64.0 KByte (default)
-----------------------------------------------------------
Accepted connection from 127.0.0.1:54321 on port 5201
[  4] 0.0-1.0 sec    128 MBytes   1074 Mbits/sec
[  4] 1.0-2.0 sec    132 MBytes   1107 Mbits/sec
[  4] 2.0-3.0 sec    129 MBytes   1082 Mbits/sec
...
```

### Terminal 2 (Client):
```bash
$ ./iperf3_client -c 127.0.0.1 -t 10
-----------------------------------------------------------
Client connecting to 127.0.0.1, TCP port 5201
TCP window size: 128.0 KByte
-----------------------------------------------------------
[  4] local 127.0.0.1 port 54321 connected to 127.0.0.1 port 5201
[  4] 0.0-1.0 sec    128 MBytes   1074 Mbits/sec
[  4] 1.0-2.0 sec    132 MBytes   1107 Mbits/sec
[  4] 2.0-3.0 sec    129 MBytes   1082 Mbits/sec
...
[  4] 9.0-10.0 sec   131 MBytes   1099 Mbits/sec
- - - - - - - - - - - - - - - - - - - - - - - - -
[  4] 0.0-10.0 sec   1.28 GBytes   1098 Mbits/sec                  sender
[  4] 0.0-10.0 sec   1.28 GBytes   1098 Mbits/sec                  receiver

iperf Done.
```

## Performance Features

The benchmark implements several optimizations for high-performance testing:

1. **TCP_NODELAY**: Disables Nagle's algorithm for lower latency
2. **SO_KEEPALIVE**: Enables TCP keep-alive for connection monitoring
3. **SO_REUSEADDR/SO_REUSEPORT**: Allows rapid server restart and port reuse
4. **Large buffers**: Default 128KB send/receive buffers
5. **Event-driven I/O**: Uses Hohnor's efficient event loop
6. **Real-time statistics**: Reports throughput every second

## Architecture

### Key Components Used

1. **EventLoop**: Core event dispatcher ([`EventLoop::createEventLoop()`](../include/hohnor/core/EventLoop.h:39))
2. **TCPAcceptor**: Server-side socket management ([`TCPAcceptor`](../include/hohnor/net/TCPAcceptor.h:18))
3. **TCPConnector**: Client-side connection management ([`TCPConnector`](../include/hohnor/net/TCPConnector.h:14))
4. **TCPConnection**: Connection I/O operations ([`TCPConnection`](../include/hohnor/net/TCPConnection.h:28))
5. **Buffer**: Efficient data buffering ([`Buffer`](../include/hohnor/common/Buffer.h:23))

### Design Patterns

- **Event-driven**: Non-blocking I/O with callbacks
- **RAII**: Automatic resource management
- **Smart pointers**: Memory safety with shared_ptr/weak_ptr
- **Signal handling**: Graceful shutdown on Ctrl+C

## Troubleshooting

### Common Issues

1. **"Address already in use"**:
   - Wait a few seconds after stopping the server
   - Or use a different port with `-p`

2. **"Connection refused"**:
   - Make sure the server is running
   - Check firewall settings
   - Verify the correct IP address and port

3. **Low performance**:
   - Try larger buffer sizes with `-l`
   - Check system network settings
   - Monitor CPU and memory usage

### Debug Mode

To enable debug logging, modify the source code to uncomment:
```cpp
Logger::setGlobalLogLevel(Logger::LogLevel::DEBUG);
```

## Comparison with iperf3

This implementation provides:
- ✅ Basic throughput testing
- ✅ Real-time statistics
- ✅ Configurable test duration
- ✅ Configurable buffer sizes
- ✅ TCP optimizations
- ⚠️ Single stream only (parallel streams planned)
- ❌ UDP testing (TCP only)
- ❌ Bidirectional testing
- ❌ JSON output format

## Future Enhancements

- [ ] Parallel stream support (`-P` option)
- [ ] UDP testing capability
- [ ] Bidirectional testing
- [ ] JSON output format
- [ ] More detailed TCP statistics
- [ ] IPv6 support
- [ ] Bandwidth limiting