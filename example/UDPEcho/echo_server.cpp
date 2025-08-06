/**
 * UDP Echo Server example using Hohnor EventLoop and UDPSocket
 * This server listens for incoming UDP datagrams and echoes back any received data
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/net/UDPSocket.h"
#include "hohnor/net/InetAddress.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/log/Logging.h"
#include <iostream>
#include <memory>
#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <cstring>

using namespace Hohnor;

class UDPEchoServer {
private:
    EventLoop* loop_;
    std::unique_ptr<UDPListenSocket> listenSocket_;
    uint16_t port_;
    bool running_;

public:
    UDPEchoServer(EventLoop* loop, uint16_t port) 
        : loop_(loop), port_(port), running_(false) {}

    void start() {
        if (running_) {
            std::cout << "Server is already running!" << std::endl;
            return;
        }

        try {
            // Create UDP listen socket
            listenSocket_ = std::make_unique<UDPListenSocket>(loop_);
            
            // Set socket options
            listenSocket_->setReuseAddr(true);
            listenSocket_->setReusePort(true);
            
            // Bind to address and port
            listenSocket_->bindAddress(port_, false, false); // port, not loopback only, not ipv6
            
            // Set data callback for incoming UDP datagrams
            listenSocket_->setDataCallback([this]() {
                this->handleIncomingData();
            });

            // Enable the socket handler
            listenSocket_->enable();

            running_ = true;
            std::cout << "UDP Echo Server started on port " << port_ << std::endl;
            std::cout << "Waiting for UDP datagrams..." << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Failed to start server: " << e.what() << std::endl;
            throw;
        }
    }

    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        // Close listen socket
        if (listenSocket_) {
            listenSocket_->disable();
            listenSocket_.reset();
        }
        
        std::cout << "UDP Echo Server stopped." << std::endl;
    }

private:
    void handleIncomingData() {
        try {
            // Buffer to receive data
            char buffer[4096];
            InetAddress clientAddr;
            
            // Receive data from client
            ssize_t bytesReceived = listenSocket_->recvFrom(buffer, sizeof(buffer) - 1, clientAddr);
            
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                std::cout << "Received from " << clientAddr.toIpPort() 
                          << " (" << bytesReceived << " bytes): " << buffer;
                
                // Echo the data back to the sender
                ssize_t bytesSent = listenSocket_->sendTo(buffer, bytesReceived, clientAddr);
                
                if (bytesSent == bytesReceived) {
                    std::cout << "Echoed back " << bytesSent << " bytes to " 
                              << clientAddr.toIpPort() << std::endl;
                } else {
                    std::cerr << "Failed to echo all data back to " 
                              << clientAddr.toIpPort() << " (sent " << bytesSent 
                              << " of " << bytesReceived << " bytes)" << std::endl;
                }
            } else if (bytesReceived == 0) {
                std::cout << "Received empty datagram" << std::endl;
            } else {
                // Error receiving
                std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling incoming data: " << e.what() << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    uint16_t port = 8080;
    
    // Parse command line arguments
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
        if (port == 0) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            return 1;
        }
    }

    std::cout << "=== Hohnor UDP Echo Server ===" << std::endl;
    std::cout << "Starting server on port " << port << std::endl;
    std::cout << "Usage: " << argv[0] << " [port]" << std::endl;
    std::cout << "==============================" << std::endl;

    try {
        // Create event loop
        EventLoop loop;
        
        // Create UDP echo server
        UDPEchoServer server(&loop, port);
        
        // Set up signal handling for graceful shutdown
        auto signalHandler = loop.handleSignal(SIGINT, SignalAction::Handled, [&]() {
            std::cout << "\nReceived SIGINT (Ctrl+C), shutting down server..." << std::endl;
            server.stop();
            loop.endLoop();
        });
        
        // Start the server
        server.start();
        
        // Run the event loop
        loop.loop();
        
        std::cout << "Server shutdown complete." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}