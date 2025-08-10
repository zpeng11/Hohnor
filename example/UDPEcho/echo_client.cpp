/**
 * UDP Echo Client example using Hohnor EventLoop and UDPSocket
 * This client sends hello messages with timestamps to the echo server
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/net/UDPSocket.h"
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

class UDPEchoClient {
private:
    EventLoopPtr loop_;
    std::unique_ptr<UDPSocket> socket_;
    InetAddress serverAddr_;
    bool running_;
    int messageCount_;

public:
    UDPEchoClient(EventLoopPtr loop, const std::string& host, uint16_t port)
        : loop_(loop), serverAddr_(host, port), running_(false), messageCount_(0) {}

    void start() {
        if (running_) {
            std::cout << "Client is already running!" << std::endl;
            return;
        }

        try {
            // Create UDP socket
            socket_ = std::make_unique<UDPSocket>(loop_);
            
            // Set up read callback for receiving echo responses
            socket_->setReadCallback([this]() {
                this->handleServerResponse();
            });

            socket_->setErrorCallback([this]() {
                this->handleError();
            });

            // Enable socket handler
            socket_->enable();

            running_ = true;
            
            std::cout << "UDP Echo Client started, sending to " << serverAddr_.toIpPort() << std::endl;
            
            // Start sending messages immediately
            sendMessage();

        } catch (const std::exception& e) {
            std::cerr << "Failed to start client: " << e.what() << std::endl;
            throw;
        }
    }

    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        if (socket_) {
            socket_->disable();
            socket_.reset();
        }
        
        std::cout << "UDP Echo Client stopped." << std::endl;
    }

private:
    void scheduleNextMessage() {
        if (!running_) return;
        
        // Schedule next message in 2 seconds
        loop_->addTimer([this]() {
            this->sendMessage();
        }, addTime(Timestamp::now(), 2.0));
    }

    void sendMessage() {
        if (!running_) return;
        
        try {
            // Create message with timestamp
            Timestamp now = Timestamp::now();
            std::string message = "Hello from UDP client #" + std::to_string(++messageCount_) 
                                + " at " + now.toFormattedString() + "\n";
            
            std::cout << "Sending to " << serverAddr_.toIpPort() << ": " << message;
            
            // Send message to server using UDP
            ssize_t bytesSent = socket_->sendTo(message.c_str(), message.length(), serverAddr_);
            
            if (bytesSent == static_cast<ssize_t>(message.length())) {
                std::cout << "Sent " << bytesSent << " bytes successfully" << std::endl;
            } else {
                std::cerr << "Failed to send complete message (sent " << bytesSent 
                          << " of " << message.length() << " bytes)" << std::endl;
            }
            
            // Schedule next message
            scheduleNextMessage();
            
        } catch (const std::exception& e) {
            std::cerr << "Error sending message: " << e.what() << std::endl;
            handleError();
        }
    }

    void handleServerResponse() {
        if (!running_) return;
        
        try {
            // Read response from server
            char buffer[4096];
            InetAddress fromAddr;
            ssize_t bytesReceived = socket_->recvFrom(buffer, sizeof(buffer) - 1, fromAddr);
            
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                std::cout << "Received echo from " << fromAddr.toIpPort() 
                          << " (" << bytesReceived << " bytes): " << buffer;
            } else if (bytesReceived == 0) {
                std::cout << "Received empty datagram from server" << std::endl;
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

    void handleError() {
        std::cerr << "Socket error occurred" << std::endl;
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

    std::cout << "=== Hohnor UDP Echo Client ===" << std::endl;
    std::cout << "Sending to " << host << ":" << port << std::endl;
    std::cout << "Usage: " << argv[0] << " [host] [port]" << std::endl;
    std::cout << "==============================" << std::endl;

    try {
        // Create event loop
        auto loop = EventLoop::create();

        // Create UDP echo client
        UDPEchoClient client(loop, host, port);

        // Set up signal handling for graceful shutdown
        loop->handleSignal(SIGINT, SignalAction::Handled, [&]() {
            std::cout << "\nReceived SIGINT (Ctrl+C), shutting down client..." << std::endl;
            client.stop();
            loop->endLoop();
        });
        
        // Start the client
        client.start();
        
        // Run the event loop
        loop->loop();

        std::cout << "Client shutdown complete." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}