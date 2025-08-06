# UDP Echo Example

This example demonstrates how to use the Hohnor library to create UDP-based client-server applications. It consists of a UDP echo server that receives datagrams and echoes them back, and a UDP echo client that sends periodic hello messages with timestamps.

## Overview

### UDP Echo Server (`echo_server.cpp`)
- Uses [`UDPListenSocket`](../../include/hohnor/net/UDPSocket.h) to listen for incoming UDP datagrams
- Binds to a specified port and waits for incoming data
- Echoes back any received data to the sender using the connectionless UDP protocol
- Handles multiple clients simultaneously without maintaining persistent connections
- Supports graceful shutdown with SIGINT (Ctrl+C)

### UDP Echo Client (`echo_client.cpp`)
- Uses [`UDPSocket`](../../include/hohnor/net/UDPSocket.h) to send UDP datagrams
- Sends periodic hello messages with timestamps every 2 seconds
- Receives and displays echo responses from the server
- Demonstrates timer-based message scheduling using [`EventLoop::addTimer()`](../../include/hohnor/core/EventLoop.h)
- Supports graceful shutdown with SIGINT (Ctrl+C)

## Key Differences from TCP Echo Example

1. **Connectionless Communication**: UDP doesn't establish connections like TCP. Each message is sent as an independent datagram.

2. **No Accept Mechanism**: The server doesn't need to accept connections. It directly receives data from any client using `recvFrom()`.

3. **Address-based Communication**: Each UDP operation includes the sender/receiver address, allowing the server to respond to the correct client.

4. **Stateless Server**: The server doesn't maintain per-client state or handlers. All communication happens through a single socket.

## Building

From the project root directory:

```bash
# Build the main Hohnor library first
mkdir -p build && cd build
cmake ..
make

# Build the UDP Echo example
cd ../example/UDPEcho
mkdir -p build && cd build
cmake ..
make
```

This will create two executables:
- `udp_echo_server` - The UDP echo server
- `udp_echo_client` - The UDP echo client

## Usage

### Running the Server

```bash
# Run server on default port 8080
./udp_echo_server

# Run server on custom port
./udp_echo_server 9090
```

The server will start listening for UDP datagrams and display:
```
=== Hohnor UDP Echo Server ===
Starting server on port 8080
Usage: ./udp_echo_server [port]
==============================
UDP Echo Server started on port 8080
Waiting for UDP datagrams...
```

### Running the Client

```bash
# Connect to server on localhost:8080
./udp_echo_client

# Connect to custom host and port
./udp_echo_client 192.168.1.100 9090
```

The client will start sending periodic messages and display:
```
=== Hohnor UDP Echo Client ===
Sending to 127.0.0.1:8080
Usage: ./udp_echo_client [host] [port]
==============================
UDP Echo Client started, sending to 127.0.0.1:8080
Sending to 127.0.0.1:8080: Hello from UDP client #1 at 2024-01-15 10:30:45.123456
Sent 65 bytes successfully
Received echo from 127.0.0.1:8080 (65 bytes): Hello from UDP client #1 at 2024-01-15 10:30:45.123456
```

## Testing

You can test the UDP echo functionality in several ways:

1. **Basic Echo Test**: Start the server, then start the client. You should see the client sending messages and receiving echoes.

2. **Multiple Clients**: Start one server and multiple clients. Each client will send messages independently, and the server will echo back to each one.

3. **Manual Testing with netcat**: You can also test the server manually using netcat:
   ```bash
   # Send a test message to the server
   echo "Hello UDP Server" | nc -u localhost 8080
   ```

4. **Cross-Platform Testing**: Test communication between clients and servers running on different machines by specifying the server's IP address.

## Implementation Details

### Server Architecture
- Uses [`UDPListenSocket::setDataCallback()`](../../include/hohnor/net/UDPSocket.h) to handle incoming data
- Calls [`recvFrom()`](../../include/hohnor/net/UDPSocket.h) to receive data and get sender address
- Uses [`sendTo()`](../../include/hohnor/net/UDPSocket.h) to echo data back to the sender
- Handles errors gracefully and continues serving other clients

### Client Architecture
- Uses [`UDPSocket::sendTo()`](../../include/hohnor/net/UDPSocket.h) to send messages to the server
- Uses [`Timestamp::now()`](../../include/hohnor/time/Timestamp.h) and [`toFormattedString()`](../../include/hohnor/time/Timestamp.h) for timestamped messages
- Uses [`EventLoop::addTimer()`](../../include/hohnor/core/EventLoop.h) for periodic message sending
- Sets up read callback to receive echo responses

### Error Handling
Both client and server include comprehensive error handling:
- Socket creation and binding errors
- Send/receive operation errors
- Signal handling for graceful shutdown
- Exception handling with informative error messages

## UDP vs TCP Considerations

When using this UDP example, keep in mind:

1. **No Delivery Guarantee**: UDP doesn't guarantee message delivery or order
2. **No Flow Control**: UDP doesn't provide automatic flow control like TCP
3. **Message Boundaries**: UDP preserves message boundaries (each send corresponds to one receive)
4. **Lower Overhead**: UDP has less protocol overhead than TCP
5. **Suitable for**: Real-time applications, gaming, streaming, where speed is more important than reliability

This example is ideal for learning UDP programming with the Hohnor library and can serve as a foundation for more complex UDP-based applications.