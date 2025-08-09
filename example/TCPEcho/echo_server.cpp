/**
 * TCP Echo Server example using Hohnor EventLoop and TCPSocket
 * This server listens for incoming connections and echoes back any received data
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/net/TCPAcceptor.h"
#include "hohnor/net/InetAddress.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/log/Logging.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <cstring>

using namespace Hohnor;

class EchoServer {
private:
    std::shared_ptr<EventLoop> loop_;
    std::shared_ptr<TCPAcceptor> listenSocket_;
    std::unordered_map<int, std::shared_ptr<IOHandler>> clients_;
    uint16_t port_;
    bool running_;

public:
    EchoServer(std::shared_ptr<EventLoop> loop, uint16_t port) 
        : loop_(loop), port_(port), running_(false) {}

    void start() {
        // Logger::setGlobalLogLevel(Logger::LogLevel::DEBUG);
        if (running_) {
            std::cout << "Server is already running!" << std::endl;
            return;
        }

        try {
            // Create TCP listen socket
            listenSocket_ = std::make_shared<TCPAcceptor>(loop_);

            // Set socket options
            listenSocket_->setReuseAddr(true);
            listenSocket_->setReusePort(true);
            
            // Bind to address and port
            listenSocket_->bindAddress(port_, false, false); // port, not loopback only, not ipv6
            
            // Start listening
            listenSocket_->listen();
            
            // Set accept callback
            listenSocket_->setAcceptCallback(std::bind(&EchoServer::handleNewConnection, this, std::placeholders::_1, std::placeholders::_2));

            running_ = true;
            std::cout << "Echo Server started on port " << port_ << std::endl;
            std::cout << "Waiting for connections..." << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Failed to start server: " << e.what() << std::endl;
            throw;
        }
    }

    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        // Close all client connections
        for (auto& pair : clients_) {
            pair.second->disable();
        }
        clients_.clear();
        
        // Close listen socket
        if (listenSocket_) {
            listenSocket_.reset();
        }
        
        std::cout << "Echo Server stopped." << std::endl;
    }

private:
    void handleNewConnection(std::shared_ptr<IOHandler> clientHandler, InetAddress clientAddr) {
        try {
            
            if (!clientHandler) {
                std::cerr << "Failed to accept connection" << std::endl;
                return;
            }

            int clientFd = clientHandler->fd();
            std::cout << "New client connected from " << clientAddr.toIpPort() 
                      << " (fd: " << clientFd << ")" << std::endl;

            // Store client handler
            clients_[clientFd] = clientHandler;

            // Set up client callbacks
            clientHandler->setReadCallback([this, clientFd]() {
                this->handleClientData(clientFd);
            });

            clientHandler->setCloseCallback([this, clientFd]() {
                this->handleClientDisconnect(clientFd);
            });

            clientHandler->setErrorCallback([this, clientFd]() {
                this->handleClientError(clientFd);
            });

            // Enable the client handler
            clientHandler->enable();

        } catch (const std::exception& e) {
            std::cerr << "Error handling new connection: " << e.what() << std::endl;
        }
    }

    void handleClientData(int clientFd) {
        auto it = clients_.find(clientFd);
        if (it == clients_.end()) {
            return;
        }

        auto clientHandler = it->second;
        
        try {
            // Read data from client
            char buffer[4096];
            ssize_t bytesRead = ::read(clientFd, buffer, sizeof(buffer) - 1);
            
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::cout << "Received from client " << clientFd << ": " << buffer;
                
                // Echo the data back
                ssize_t bytesWritten = ::write(clientFd, buffer, bytesRead);
                if (bytesWritten != bytesRead) {
                    std::cerr << "Failed to echo all data to client " << clientFd << std::endl;
                }
            } else if (bytesRead == 0) {
                // Client closed connection
                std::cout << "Client " << clientFd << " closed connection" << std::endl;
                handleClientDisconnect(clientFd);
            } else {
                // Error reading
                std::cerr << "Error reading from client " << clientFd << ": " << strerror(errno) << std::endl;
                handleClientError(clientFd);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling client data: " << e.what() << std::endl;
            handleClientError(clientFd);
        }
    }

    void handleClientDisconnect(int clientFd) {
        std::cout << "Client " << clientFd << " disconnected" << std::endl;
        
        auto it = clients_.find(clientFd);
        if (it != clients_.end()) {
            it->second->disable();
            clients_.erase(it);
        }
    }

    void handleClientError(int clientFd) {
        std::cerr << "Error with client " << clientFd << std::endl;
        handleClientDisconnect(clientFd);
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

    std::cout << "=== Hohnor TCP Echo Server ===" << std::endl;
    std::cout << "Starting server on port " << port << std::endl;
    std::cout << "Usage: " << argv[0] << " [port]" << std::endl;
    std::cout << "==============================" << std::endl;

    try {
        // Create event loop
        auto loop = EventLoop::createEventLoop();
        
        // Create echo server
        EchoServer server(loop, port);

        // Set up signal handling for graceful shutdown
        loop->handleSignal(SIGINT, SignalAction::Handled, [&]() {
            std::cout << "\nReceived SIGINT (Ctrl+C), shutting down server..." << std::endl;
            server.stop();
            loop->endLoop();
        });
        
        // Start the server
        server.start();
        
        // Run the event loop
        loop->loop();

        std::cout << "Server shutdown complete." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}