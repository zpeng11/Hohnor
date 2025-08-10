/**
 * TCP Echo Client example using Hohnor EventLoop and TCPConnector
 * This client connects to the echo server and sends hello messages with timestamps
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/net/TCPConnector.h"
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
    std::shared_ptr<EventLoop> loop_;
    std::shared_ptr<TCPConnector> connector_;
    std::shared_ptr<TCPConnection> connection_;
    std::string serverHost_;
    uint16_t serverPort_;
    bool connected_;
    bool running_;
    int messageCount_;

public:
    EchoClient(std::shared_ptr<EventLoop> loop, const std::string& host, uint16_t port)
        : loop_(loop), serverHost_(host), serverPort_(port),
          connected_(false), running_(false), messageCount_(0) {}

    void start() {
        if (running_) {
            std::cout << "Client is already running!" << std::endl;
            return;
        }
        Logger::setGlobalLogLevel(Logger::LogLevel::DEBUG);
        try {
            // Create TCP connector
            InetAddress serverAddr(serverHost_, serverPort_);
            connector_ = std::make_shared<TCPConnector>(loop_, serverAddr);
            
            // Set up connection callbacks
            connector_->setNewConnectionCallback([this](std::shared_ptr<TCPConnection> connection) {
                this->handleNewConnection(connection);
            });

            connector_->setRetryConnectionCallback([this]() {
                std::cout << "Retrying connection to server..." << std::endl;
            });

            connector_->setFailedConnectionCallback([this]() {
                std::cerr << "Failed to connect to server after all retries" << std::endl;
                this->handleError();
            });

            // Configure retry behavior (optional)
            connector_->setRetries(5);  // Try 5 times before giving up
            connector_->setRetryConstantDelay(1000);  // 1 second between retries

            running_ = true;

            LOG_DEBUG << "Finished setup values";
            
            std::cout << "Connecting to server " << serverAddr.toIpPort() << "..." << std::endl;
            
            // Start connection process
            connector_->start();

        } catch (const std::exception& e) {
            std::cerr << "Failed to start client: " << e.what() << std::endl;
            throw;
        }
    }

    void stop() {
        if (!running_) return;
        
        running_ = false;
        connected_ = false;
        
        if (connection_) {
            connection_->forceClose();
            connection_.reset();
            LOG_DEBUG << "Connection handler reset and disabled";
        }
        
        if (connector_) {
            connector_->stop();
            connector_.reset();
            LOG_DEBUG << "Connector stopped and reset";
        }
        
        std::cout << "Client stopped." << std::endl;
    }

private:
    void handleNewConnection(std::shared_ptr<TCPConnection> connection) {
        std::cout << "Connected to server successfully!" << std::endl;
        
        connection_ = connection;
        connected_ = true;
        
        // Set up callbacks for the connection
        connection_->setReadCompleteCallback([this](TCPConnectionWeakPtr weakConn) {
            if (auto conn = weakConn.lock()) {
                this->handleServerResponse();
            }
        });

        connection_->setCloseCallback([this]() {
            this->handleDisconnect();
        });

        connection_->setErrorCallback([this]() {
            this->handleError();
        });

        connection_->setWriteCompleteCallback([this](TCPConnectionWeakPtr weakConn) {
            if (auto conn = weakConn.lock()) {
                this->sendMessage();
            }
        });
        
        // Start reading from the connection
        connection_->readRaw();

        this->sendMessage();
        
        // Start sending messages periodically
        // scheduleNextMessage();
    }

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
            
            // Send message to server using TCPConnection's write method
            connection_->write(message);
            
            // Schedule next message
            // scheduleNextMessage();
            
        } catch (const std::exception& e) {
            std::cerr << "Error sending message: " << e.what() << std::endl;
            handleError();
        }
    }

    void handleServerResponse() {
        if (!connected_) return;
        
        try {
            // Get data from the read buffer
            Buffer& readBuffer = connection_->getReadBuffer();
            
            if (readBuffer.readableBytes() > 0) {
                // Get the data as string
                std::string data = readBuffer.retrieveAllAsString();
                std::cout << "Received echo: " << data;
                
                // Continue reading
                connection_->readRaw();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling server response: " << e.what() << std::endl;
            handleError();
        }
    }

    void handleDisconnect() {
        std::cout << "Disconnected from server" << std::endl;
        LOG_DEBUG << "Handling disconnect for EchoClient";
        connected_ = false;
        running_ = false;
        loop_->endLoop();
    }

    void handleError() {
        std::cerr << "Socket error occurred" << std::endl;
        LOG_DEBUG << "Handling error for EchoClient";
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
        auto loop = EventLoop::createEventLoop();

        // Create echo client
        EchoClient client(loop, host, port);

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

        LOG_DEBUG << "Event loop exited";
        
        std::cout << "Client shutdown complete." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}