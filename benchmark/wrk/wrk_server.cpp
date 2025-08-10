/**
 * wrk-compatible HTTP Server Benchmark using Hohnor EventLoop and TCPAcceptor
 * This server is designed to work with wrk HTTP benchmarking tool
 * It provides high-performance HTTP responses for load testing
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/net/TCPAcceptor.h"
#include "hohnor/net/InetAddress.h"
#include "hohnor/net/TCPConnection.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/log/Logging.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <atomic>
#include <iomanip>
#include <sstream>

using namespace Hohnor;

struct ConnectionStats {
    uint64_t requestsHandled = 0;
    uint64_t bytesReceived = 0;
    uint64_t bytesSent = 0;
    Timestamp startTime;
    Timestamp lastReportTime;
    uint64_t lastRequestsHandled = 0;
    bool active = true;
    
    ConnectionStats() : startTime(Timestamp::now()), lastReportTime(Timestamp::now()) {}
};

class WrkHttpServer {
private:
    std::shared_ptr<EventLoop> loop_;
    std::shared_ptr<TCPAcceptor> listenSocket_;
    std::unordered_map<int, TCPConnectionPtr> clients_;
    std::unordered_map<int, ConnectionStats> clientStats_;
    uint16_t port_;
    bool running_;
    std::atomic<uint64_t> totalRequests_;
    std::atomic<uint64_t> totalBytesReceived_;
    std::atomic<uint64_t> totalBytesSent_;
    Timestamp serverStartTime_;
    
    // HTTP response templates
    std::string httpResponse200_;
    std::string httpResponse404_;
    
    // Statistics reporting
    static constexpr double REPORT_INTERVAL = 5.0; // Report every 5 seconds

public:
    WrkHttpServer(std::shared_ptr<EventLoop> loop, uint16_t port) 
        : loop_(loop), port_(port), running_(false), 
          totalRequests_(0), totalBytesReceived_(0), totalBytesSent_(0),
          serverStartTime_(Timestamp::now()) {
        
        // Prepare HTTP response templates
        prepareHttpResponses();
    }

    void start() {
        if (running_) {
            std::cout << "Server is already running!" << std::endl;
            return;
        }

        try {
            // Create TCP listen socket
            listenSocket_ = std::make_shared<TCPAcceptor>(loop_);

            // Set socket options for high performance
            listenSocket_->setReuseAddr(true);
            listenSocket_->setReusePort(true);
            listenSocket_->setTCPNoDelay(true);  // Disable Nagle's algorithm
            listenSocket_->setKeepAlive(true);   // Enable keep-alive
            
            // Bind to address and port
            listenSocket_->bindAddress(port_, false, false); // port, not loopback only, not ipv6
            
            // Start listening
            listenSocket_->listen();
            
            // Set accept callback
            listenSocket_->setAcceptCallback(std::bind(&WrkHttpServer::handleNewConnection, this, std::placeholders::_1));

            running_ = true;
            serverStartTime_ = Timestamp::now();
            
            std::cout << "====================================================" << std::endl;
            std::cout << "wrk-compatible HTTP Server started" << std::endl;
            std::cout << "Listening on: http://localhost:" << port_ << std::endl;
            std::cout << "Ready for wrk benchmarking" << std::endl;
            std::cout << "====================================================" << std::endl;
            std::cout << "Example wrk command:" << std::endl;
            std::cout << "  wrk -t12 -c400 -d30s http://localhost:" << port_ << "/" << std::endl;
            std::cout << "====================================================" << std::endl;

            // Schedule periodic statistics reporting
            scheduleStatsReport();

        } catch (const std::exception& e) {
            std::cerr << "Failed to start server: " << e.what() << std::endl;
            throw;
        }
    }

    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        // Print final statistics
        printFinalStats();
        
        // Close all client connections
        for (auto& pair : clients_) {
            pair.second->forceClose();
        }
        clients_.clear();
        clientStats_.clear();
        
        // Close listen socket
        if (listenSocket_) {
            listenSocket_.reset();
        }
        
        std::cout << "HTTP Server stopped." << std::endl;
    }

private:
    void prepareHttpResponses() {
        // Prepare a simple HTTP 200 OK response
        std::ostringstream oss200;
        std::string body = "Hello, wrk! This is a high-performance HTTP server built with Hohnor library.";
        
        oss200 << "HTTP/1.1 200 OK\r\n"
               << "Content-Type: text/plain\r\n"
               << "Content-Length: " << body.length() << "\r\n"
               << "Connection: keep-alive\r\n"
               << "Server: Hohnor-wrk/1.0\r\n"
               << "\r\n"
               << body;
        httpResponse200_ = oss200.str();
        
        // Prepare HTTP 404 Not Found response
        std::ostringstream oss404;
        std::string body404 = "404 Not Found";
        
        oss404 << "HTTP/1.1 404 Not Found\r\n"
               << "Content-Type: text/plain\r\n"
               << "Content-Length: " << body404.length() << "\r\n"
               << "Connection: keep-alive\r\n"
               << "Server: Hohnor-wrk/1.0\r\n"
               << "\r\n"
               << body404;
        httpResponse404_ = oss404.str();
    }

    void handleNewConnection(TCPConnectionPtr clientConnection) {
        try {
            if (!clientConnection) {
                std::cerr << "Failed to accept connection" << std::endl;
                return;
            }

            int clientFd = clientConnection->fd();
            
            // Set TCP options for performance
            clientConnection->setTCPNoDelay(true);
            
            // Store client connection and initialize stats
            clients_[clientFd] = clientConnection;
            clientStats_[clientFd] = ConnectionStats();

            // Set up client callbacks
            clientConnection->setReadCompleteCallback([this, clientFd](TCPConnectionPtr conn) {
                this->handleHttpRequest(clientFd);
            });

            clientConnection->setCloseCallback([this, clientFd]() {
                this->handleClientDisconnect(clientFd);
            });

            clientConnection->setErrorCallback([this, clientFd]() {
                this->handleClientError(clientFd);
            });

            // Start reading HTTP requests
            clientConnection->readUntil("\r\n\r\n"); // Read until end of HTTP headers

        } catch (const std::exception& e) {
            std::cerr << "Error handling new connection: " << e.what() << std::endl;
        }
    }

    void handleHttpRequest(int clientFd) {
        auto it = clients_.find(clientFd);
        if (it == clients_.end()) {
            return;
        }

        auto clientConnection = it->second;
        auto& stats = clientStats_[clientFd];
        
        try {
            // Get data from the read buffer
            Buffer& readBuffer = clientConnection->getReadBuffer();
            
            if (readBuffer.readableBytes() > 0) {
                std::string request = readBuffer.retrieveAllAsString();
                size_t bytesReceived = request.length();
                
                // Update statistics
                stats.bytesReceived += bytesReceived;
                stats.requestsHandled++;
                totalBytesReceived_ += bytesReceived;
                totalRequests_++;
                
                // Parse HTTP request (simple parsing)
                std::string response = parseAndGenerateResponse(request);
                
                // Send HTTP response
                clientConnection->write(response);
                stats.bytesSent += response.length();
                totalBytesSent_ += response.length();
                
                // Continue reading for keep-alive connections
                clientConnection->readUntil("\r\n\r\n");
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling HTTP request: " << e.what() << std::endl;
            handleClientError(clientFd);
        }
    }

    std::string parseAndGenerateResponse(const std::string& request) {
        // Simple HTTP request parsing
        std::istringstream iss(request);
        std::string method, path, version;
        iss >> method >> path >> version;
        
        // For wrk benchmarking, we mainly handle GET requests
        if (method == "GET") {
            if (path == "/" || path == "/index.html" || path == "/test") {
                return httpResponse200_;
            } else {
                return httpResponse404_;
            }
        }
        
        // Default response for other methods
        return httpResponse200_;
    }

    void handleClientDisconnect(int clientFd) {
        auto it = clientStats_.find(clientFd);
        if (it != clientStats_.end()) {
            auto& stats = it->second;
            stats.active = false;
        }
        
        clients_.erase(clientFd);
        clientStats_.erase(clientFd);
    }

    void handleClientError(int clientFd) {
        handleClientDisconnect(clientFd);
    }

    void scheduleStatsReport() {
        if (!running_) return;
        
        loop_->addTimer([this]() {
            this->printIntervalStats();
            this->scheduleStatsReport(); // Schedule next report
        }, addTime(Timestamp::now(), REPORT_INTERVAL));
    }

    void printIntervalStats() {
        if (!running_) return;
        
        Timestamp now = Timestamp::now();
        double totalDuration = timeDifference(now, serverStartTime_);
        
        uint64_t currentRequests = totalRequests_.load();
        uint64_t currentBytesReceived = totalBytesReceived_.load();
        uint64_t currentBytesSent = totalBytesSent_.load();
        
        double requestsPerSec = currentRequests / totalDuration;
        double mbpsReceived = (currentBytesReceived * 8.0) / (totalDuration * 1000000.0);
        double mbpsSent = (currentBytesSent * 8.0) / (totalDuration * 1000000.0);
        
        std::cout << "[" << std::fixed << std::setprecision(1) << totalDuration << "s] "
                  << "Requests: " << currentRequests 
                  << " (" << std::setprecision(0) << requestsPerSec << " req/s), "
                  << "Connections: " << clients_.size()
                  << ", RX: " << std::setprecision(1) << mbpsReceived << " Mbps"
                  << ", TX: " << mbpsSent << " Mbps" << std::endl;
    }

    void printFinalStats() {
        Timestamp now = Timestamp::now();
        double totalDuration = timeDifference(now, serverStartTime_);
        
        uint64_t finalRequests = totalRequests_.load();
        uint64_t finalBytesReceived = totalBytesReceived_.load();
        uint64_t finalBytesSent = totalBytesSent_.load();
        
        double avgRequestsPerSec = finalRequests / totalDuration;
        double avgMbpsReceived = (finalBytesReceived * 8.0) / (totalDuration * 1000000.0);
        double avgMbpsSent = (finalBytesSent * 8.0) / (totalDuration * 1000000.0);
        
        std::cout << "====================================================" << std::endl;
        std::cout << "Final Server Statistics:" << std::endl;
        std::cout << "Total Duration: " << std::fixed << std::setprecision(2) << totalDuration << " seconds" << std::endl;
        std::cout << "Total Requests: " << finalRequests << std::endl;
        std::cout << "Average Requests/sec: " << std::setprecision(0) << avgRequestsPerSec << std::endl;
        std::cout << "Total Bytes Received: " << formatBytes(finalBytesReceived) << std::endl;
        std::cout << "Total Bytes Sent: " << formatBytes(finalBytesSent) << std::endl;
        std::cout << "Average RX Throughput: " << std::setprecision(1) << avgMbpsReceived << " Mbps" << std::endl;
        std::cout << "Average TX Throughput: " << avgMbpsSent << " Mbps" << std::endl;
        std::cout << "====================================================" << std::endl;
    }

    std::string formatBytes(uint64_t bytes) {
        if (bytes >= 1000000000) {
            return std::to_string(bytes / 1000000) + " MB";
        } else if (bytes >= 1000000) {
            return std::to_string(bytes / 1000) + " KB";
        } else {
            return std::to_string(bytes) + " Bytes";
        }
    }
};

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -p, --port <port>     Server port to listen on (default: 8080)" << std::endl;
    std::cout << "  -h, --help            Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program << " -p 8080" << std::endl;
    std::cout << std::endl;
    std::cout << "Then test with wrk:" << std::endl;
    std::cout << "  wrk -t12 -c400 -d30s http://localhost:8080/" << std::endl;
}

int main(int argc, char* argv[]) {
    uint16_t port = 8080;  // Default port
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                port = static_cast<uint16_t>(std::atoi(argv[++i]));
                if (port == 0) {
                    std::cerr << "Invalid port number: " << argv[i] << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Option " << arg << " requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    try {
        // Create event loop
        auto loop = EventLoop::createEventLoop();
        
        // Create wrk HTTP server
        WrkHttpServer server(loop, port);

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
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}