/**
 * TCP Echo Client example using Hohnor EventLoop and TCPSocket
 * This client connects to the echo server and sends hello messages with timestamps
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/net/Socket.h"
#include "hohnor/net/InetAddress.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/log/Logging.h"
#include <iostream>
#include <memory>
#include <string>
#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <cstring>

using namespace Hohnor;

class EchoClient {
private:
    EventLoop* loop_;
    std::unique_ptr<Socket> socket_;
    std::string serverHost_;
    uint16_t serverPort_;
    bool connected_;
    bool running_;
    int messageCount_;

public:
    EchoClient(EventLoop* loop, const std::string& host, uint16_t port)
        : loop_(loop), serverHost_(host), serverPort_(port), 
          connected_(false), running_(false), messageCount_(0) {}

    void start() {
        if (running_) {
            std::cout << "Client is already running!" << std::endl;
            return;
        }
        // Logger::setGlobalLogLevel(Logger::LogLevel::DEBUG);
        try {
            // Create TCP socket
            socket_ = std::make_unique<Socket>(loop_, AF_INET, SOCK_STREAM);
            
            // Set up callbacks
            socket_->setReadCallback([this]() {
                this->handleServerResponse();
            });

            socket_->setCloseCallback([this]() {
                this->handleDisconnect();
            });

            socket_->setErrorCallback([this]() {
                this->handleError();
            });

            // Connect to server
            InetAddress serverAddr(serverHost_, serverPort_);
            std::cout << "Connecting to server " << serverAddr.toIpPort() << "..." << std::endl;
            
            int conRet = socket_->connect(serverAddr, false);
            
            // Check if connection was successful
            if (conRet != 0) {
                std::cerr << "Failed to connect: " << socket_->getSockErrorStr() << std::endl;
                loop_->endLoop();
                return;
            }

            connected_ = true;
            running_ = true;
            
            std::cout << "Connected to server successfully!" << std::endl;
            
            // Enable socket handler
            socket_->enable();
            
            // Start sending messages periodically
            scheduleNextMessage();

        } catch (const std::exception& e) {
            std::cerr << "Failed to start client: " << e.what() << std::endl;
            throw;
        }
    }

    void stop() {
        if (!running_) return;
        
        running_ = false;
        connected_ = false;
        
        if (socket_) {
            socket_->disable();
        }
        
        if (socket_) {
            socket_.reset();
        }
        
        std::cout << "Client stopped." << std::endl;
    }

private:
    void scheduleNextMessage() {
        if (!running_ || !connected_) return;
        
        // Schedule next message in 2 seconds
        loop_->addTimer([this]() {
            this->sendMessage();
        }, addTime(Timestamp::now(), 2.0));
    }

    void sendMessage() {
        if (!running_ || !connected_) return;
        
        try {
            // Create message with timestamp
            Timestamp now = Timestamp::now();
            std::string message = "Hello from client #" + std::to_string(++messageCount_) 
                                + " at " + now.toFormattedString() + "\n";
            
            std::cout << "Sending: " << message;
            
            // Send message to server
            int fd = socket_->fd();
            ssize_t bytesWritten = ::write(fd, message.c_str(), message.length());
            
            if (bytesWritten != static_cast<ssize_t>(message.length())) {
                std::cerr << "Failed to send complete message" << std::endl;
                handleError();
                return;
            }
            
            // Schedule next message
            scheduleNextMessage();
            
        } catch (const std::exception& e) {
            std::cerr << "Error sending message: " << e.what() << std::endl;
            handleError();
        }
    }

    void handleServerResponse() {
        if (!connected_) return;
        
        try {
            // Read response from server
            char buffer[4096];
            int fd = socket_->fd();
            ssize_t bytesRead = ::read(fd, buffer, sizeof(buffer) - 1);
            
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::cout << "Received echo: " << buffer;
            } else if (bytesRead == 0) {
                // Server closed connection
                std::cout << "Server closed connection" << std::endl;
                handleDisconnect();
            } else {
                // Error reading
                std::cerr << "Error reading from server: " << strerror(errno) << std::endl;
                handleError();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling server response: " << e.what() << std::endl;
            handleError();
        }
    }

    void handleDisconnect() {
        std::cout << "Disconnected from server" << std::endl;
        connected_ = false;
        running_ = false;
        loop_->endLoop();
    }

    void handleError() {
        std::cerr << "Socket error occurred" << std::endl;
        connected_ = false;
        running_ = false;
        loop_->endLoop();
    }
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    uint16_t port = 8080;
    
    // Parse command line arguments
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = static_cast<uint16_t>(std::atoi(argv[2]));
        if (port == 0) {
            std::cerr << "Invalid port number: " << argv[2] << std::endl;
            return 1;
        }
    }

    std::cout << "=== Hohnor TCP Echo Client ===" << std::endl;
    std::cout << "Connecting to " << host << ":" << port << std::endl;
    std::cout << "Usage: " << argv[0] << " [host] [port]" << std::endl;
    std::cout << "==============================" << std::endl;

    try {
        // Create event loop
        EventLoop loop;
        
        // Create echo client
        EchoClient client(&loop, host, port);
        
        // Set up signal handling for graceful shutdown
        auto signalHandler = loop.handleSignal(SIGINT, SignalAction::Handled, [&]() {
            std::cout << "\nReceived SIGINT (Ctrl+C), shutting down client..." << std::endl;
            client.stop();
            loop.endLoop();
        });
        
        // Start the client
        client.start();
        
        // Run the event loop
        loop.loop();
        
        std::cout << "Client shutdown complete." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}