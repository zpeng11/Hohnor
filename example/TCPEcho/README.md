# TCP Echo Example

This example demonstrates how to use the Hohnor library's [`EventLoop`](../../include/hohnor/core/EventLoop.h:33) and [`TCPSocket`](../../include/hohnor/net/TCPSocket.h:17) classes to create a simple TCP echo server and client.

## Overview

The example consists of two programs:

1. **Echo Server** (`echo_server`) - A TCP server that listens for incoming connections and echoes back any received data
2. **Echo Client** (`echo_client`) - A TCP client that connects to the server and periodically sends hello messages with timestamps

## Features

### Echo Server
- Uses [`TCPListenSocket`](../../include/hohnor/net/TCPSocket.h:17) for accepting connections
- Handles multiple concurrent clients using [`EventLoop`](../../include/hohnor/core/EventLoop.h:33)
- Echoes back all received data to the sender
- Graceful shutdown on SIGINT (Ctrl+C)
- Configurable port number

### Echo Client
- Uses [`Socket`](../../include/hohnor/net/Socket.h:23) for TCP connections
- Sends periodic hello messages with [`Timestamp`](../../include/hohnor/time/Timestamp.h:16)
- Receives and displays echoed responses
- Automatic reconnection handling
- Graceful shutdown on SIGINT (Ctrl+C)
- Configurable server host and port

## Building

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler
- Built Hohnor library (libhohnor.a)

### Build Steps

1. Make sure the Hohnor library is built in the project root:
   ```bash
   cd ../../
   mkdir -p build
   cd build
   cmake ..
   make
   ```

2. Build the TCP Echo example:
   ```bash
   cd ../example/TCPEcho
   mkdir -p build
   cd build
   cmake ..
   make
   ```

This will create two executables:
- `echo_server` - The TCP echo server
- `echo_client` - The TCP echo client

## Usage

### Running the Echo Server

```bash
./echo_server [port]
```

**Parameters:**
- `port` (optional) - Port number to listen on (default: 8080)

**Example:**
```bash
# Start server on default port 8080
./echo_server

# Start server on port 9999
./echo_server 9999
```

### Running the Echo Client

```bash
./echo_client [host] [port]
```

**Parameters:**
- `host` (optional) - Server hostname or IP address (default: 127.0.0.1)
- `port` (optional) - Server port number (default: 8080)

**Example:**
```bash
# Connect to server on localhost:8080
./echo_client

# Connect to server on localhost:9999
./echo_client 127.0.0.1 9999

# Connect to remote server
./echo_client 192.168.1.100 8080
```

## Example Session

### Terminal 1 (Server):
```bash
$ ./echo_server
=== Hohnor TCP Echo Server ===
Starting server on port 8080
Usage: ./echo_server [port]
==============================
Echo Server started on port 8080
Waiting for connections...
New client connected from 127.0.0.1:54321 (fd: 4)
Received from client 4: Hello from client #1 at 2025-08-06 00:22:58.123456
Received from client 4: Hello from client #2 at 2025-08-06 00:23:00.123456
```

### Terminal 2 (Client):
```bash
$ ./echo_client
=== Hohnor TCP Echo Client ===
Connecting to 127.0.0.1:8080
Usage: ./echo_client [host] [port]
==============================
Connecting to server 127.0.0.1:8080...
Connected to server successfully!
Sending: Hello from client #1 at 2025-08-06 00:22:58.123456
Received echo: Hello from client #1 at 2025-08-06 00:22:58.123456
Sending: Hello from client #2 at 2025-08-06 00:23:00.123456
Received echo: Hello from client #2 at 2025-08-06 00:23:00.123456
```

## Key Components Used

### EventLoop
- [`EventLoop::loop()`](../../include/hohnor/core/EventLoop.h:43) - Main event processing loop
- [`EventLoop::handleIO()`](../../include/hohnor/core/EventLoop.h:66) - Register file descriptors for I/O events
- [`EventLoop::addTimer()`](../../include/hohnor/core/EventLoop.h:70) - Schedule periodic tasks
- [`EventLoop::handleSignal()`](../../include/hohnor/core/EventLoop.h:73) - Handle system signals

### TCP Socket Classes
- [`TCPListenSocket`](../../include/hohnor/net/TCPSocket.h:17) - Server-side listening socket
- [`Socket`](../../include/hohnor/net/Socket.h:23) - Client-side connection socket
- [`InetAddress`](../../include/hohnor/net/InetAddress.h:21) - Network address representation

### I/O Handling
- [`IOHandler`](../../include/hohnor/core/IOHandler.h:23) - Manages file descriptor lifecycle and events
- [`ReadCallback`](../../include/hohnor/common/Callbacks.h:12) - Callback for read events
- [`CloseCallback`](../../include/hohnor/common/Callbacks.h:14) - Callback for connection close events

### Time Management
- [`Timestamp::now()`](../../include/hohnor/time/Timestamp.h:62) - Get current time
- [`Timestamp::toFormattedString()`](../../include/hohnor/time/Timestamp.h:48) - Format timestamp as string
- [`addTime()`](../../include/hohnor/time/Timestamp.h:131) - Add time interval to timestamp

## Stopping the Programs

Both programs can be stopped gracefully by pressing **Ctrl+C** (SIGINT). They will:
- Close all active connections
- Clean up resources
- Exit cleanly

## Notes

- The client sends a new message every 2 seconds
- The server can handle multiple concurrent clients
- All network operations are non-blocking and event-driven
- Error handling includes connection failures and network errors
- Both programs use the same [`EventLoop`](../../include/hohnor/core/EventLoop.h:33) pattern for consistent event handling