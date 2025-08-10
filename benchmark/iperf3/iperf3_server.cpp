/**
 * iperf3-style TCP Server Benchmark using Hohnor EventLoop and TCPAcceptor
 * This server measures TCP throughput by receiving data from clients and reporting statistics
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

using namespace Hohnor;

struct ConnectionStats {
    uint64_t bytesReceived = 0;
    Timestamp startTime;
    Timestamp lastReportTime;
    uint64_t lastBytesReceived = 0;
    bool active = true;
    
    ConnectionStats() : startTime(Timestamp::now()), lastReportTime(Timestamp::now()) {}
};

class IPerf3Server {
private:
    std::shared_ptr<EventLoop> loop_;
    std::shared_ptr<TCPAcceptor> listenSocket_;
    std::unordered_map<int, std::shared_ptr<TCPConnection>> clients_;
    std::unordered_map<int, ConnectionStats> clientStats_;
    uint16_t port_;
    bool running_;
    int testDuration_;  // Test duration in seconds (0 = unlimited)
    std::atomic<uint64_t> totalBytesReceived_;
    Timestamp serverStartTime_;
    
    // Statistics reporting
    static constexpr double REPORT_INTERVAL = 1.0; // Report every 1 second

public:
    IPerf3Server(std::shared_ptr<EventLoop> loop, uint16_t port, int duration = 0) 
        : loop_(loop), port_(port), running_(false), testDuration_(duration), 
          totalBytesReceived_(0), serverStartTime_(Timestamp::now()) {}

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
            listenSocket_->setAcceptCallback(std::bind(&IPerf3Server::handleNewConnection, this, std::placeholders::_1));

            running_ = true;
            serverStartTime_ = Timestamp::now();
            
            std::cout << "-----------------------------------------------------------" << std::endl;
            std::cout << "Server listening on " << port_ << std::endl;
            if (testDuration_ > 0) {
                std::cout << "Test duration: " << testDuration_ << " seconds" << std::endl;
            } else {
                std::cout << "Test duration: unlimited (until Ctrl+C)" << std::endl;
            }
            std::cout << "TCP window size: 64.0 KByte (default)" << std::endl;
            std::cout << "-----------------------------------------------------------" << std::endl;

            // Schedule periodic statistics reporting
            scheduleStatsReport();
            
            // Schedule test end if duration is specified
            if (testDuration_ > 0) {
                loop_->addTimer([this]() {
                    this->endTest();
                }, addTime(Timestamp::now(), testDuration_));
            }

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
        
        std::cout << "iperf Done." << std::endl;
    }

private:
    void handleNewConnection(std::shared_ptr<TCPConnection> clientConnection) {
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
            
            std::cout << "Accepted connection from " << clientConnection->getPeerAddr().toIpPort() 
                      << " on port " << port_ << std::endl;

            // Set up client callbacks
            clientConnection->setReadCompleteCallback([this, clientFd](TCPConnectionWeakPtr weakConn) {
                if (auto conn = weakConn.lock()) {
                    this->handleClientData(clientFd);
                }
            });

            clientConnection->setCloseCallback([this, clientFd]() {
                this->handleClientDisconnect(clientFd);
            });

            clientConnection->setErrorCallback([this, clientFd]() {
                this->handleClientError(clientFd);
            });

            // Start reading from the client
            clientConnection->readRaw();

        } catch (const std::exception& e) {
            std::cerr << "Error handling new connection: " << e.what() << std::endl;
        }
    }

    void handleClientData(int clientFd) {
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
                size_t bytesRead = readBuffer.readableBytes();
                
                // Update statistics
                stats.bytesReceived += bytesRead;
                totalBytesReceived_ += bytesRead;
                
                // Clear the buffer (we don't need to process the data, just measure throughput)
                readBuffer.retrieveAll();
                
                // Continue reading
                clientConnection->readRaw();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling client data: " << e.what() << std::endl;
            handleClientError(clientFd);
        }
    }

    void handleClientDisconnect(int clientFd) {
        auto it = clientStats_.find(clientFd);
        if (it != clientStats_.end()) {
            auto& stats = it->second;
            stats.active = false;
            
            // Print final stats for this connection
            Timestamp now = Timestamp::now();
            double duration = timeDifference(now, stats.startTime);
            double throughputMbps = (stats.bytesReceived * 8.0) / (duration * 1000000.0);
            
            std::cout << "[  " << clientFd << "] " 
                      << std::fixed << std::setprecision(1) << "0.0-" << duration << " sec  "
                      << std::setw(8) << formatBytes(stats.bytesReceived) << "  "
                      << std::setw(8) << std::setprecision(1) << throughputMbps << " Mbits/sec" << std::endl;
        }
        
        clients_.erase(clientFd);
        clientStats_.erase(clientFd);
        
        // If no more clients and test duration is set, we might want to continue waiting
        if (clients_.empty() && testDuration_ == 0) {
            std::cout << "No active connections. Waiting for new connections..." << std::endl;
        }
    }

    void handleClientError(int clientFd) {
        std::cerr << "Error with client " << clientFd << std::endl;
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
        if (!running_ || clients_.empty()) return;
        
        Timestamp now = Timestamp::now();
        
        for (auto& pair : clientStats_) {
            int clientFd = pair.first;
            auto& stats = pair.second;
            
            if (!stats.active) continue;
            
            double intervalDuration = timeDifference(now, stats.lastReportTime);
            uint64_t intervalBytes = stats.bytesReceived - stats.lastBytesReceived;
            double intervalThroughputMbps = (intervalBytes * 8.0) / (intervalDuration * 1000000.0);
            
            double totalDuration = timeDifference(now, stats.startTime);
            
            std::cout << "[  " << clientFd << "] " 
                      << std::fixed << std::setprecision(1) 
                      << totalDuration - intervalDuration << "-" << totalDuration << " sec  "
                      << std::setw(8) << formatBytes(intervalBytes) << "  "
                      << std::setw(8) << std::setprecision(1) << intervalThroughputMbps << " Mbits/sec" << std::endl;
            
            // Update for next interval
            stats.lastReportTime = now;
            stats.lastBytesReceived = stats.bytesReceived;
        }
    }

    void printFinalStats() {
        Timestamp now = Timestamp::now();
        double totalDuration = timeDifference(now, serverStartTime_);
        double totalThroughputMbps = (totalBytesReceived_ * 8.0) / (totalDuration * 1000000.0);
        
        std::cout << "-----------------------------------------------------------" << std::endl;
        std::cout << "Server Report:" << std::endl;
        std::cout << "[SUM] " 
                  << std::fixed << std::setprecision(1) << "0.0-" << totalDuration << " sec  "
                  << std::setw(8) << formatBytes(totalBytesReceived_) << "  "
                  << std::setw(8) << std::setprecision(1) << totalThroughputMbps << " Mbits/sec" << std::endl;
        std::cout << "-----------------------------------------------------------" << std::endl;
    }

    void endTest() {
        std::cout << "\nTest completed after " << testDuration_ << " seconds." << std::endl;
        stop();
        loop_->endLoop();
    }

    std::string formatBytes(uint64_t bytes) {
        if (bytes >= 1000000000) {
            return std::to_string(bytes / 1000000) + " MBytes";
        } else if (bytes >= 1000000) {
            return std::to_string(bytes / 1000) + " KBytes";
        } else {
            return std::to_string(bytes) + " Bytes";
        }
    }
};

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -s, --server          Run in server mode" << std::endl;
    std::cout << "  -p, --port <port>     Server port to listen on (default: 5201)" << std::endl;
    std::cout << "  -t, --time <sec>      Time in seconds to run (default: unlimited)" << std::endl;
    std::cout << "  -h, --help            Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program << " -s -p 5201 -t 10" << std::endl;
}

int main(int argc, char* argv[]) {
    uint16_t port = 5201;  // Default iperf3 port
    int testDuration = 0;  // 0 = unlimited
    bool serverMode = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-s" || arg == "--server") {
            serverMode = true;
        } else if (arg == "-p" || arg == "--port") {
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
        } else if (arg == "-t" || arg == "--time") {
            if (i + 1 < argc) {
                testDuration = std::atoi(argv[++i]);
                if (testDuration < 0) {
                    std::cerr << "Invalid test duration: " << argv[i] << std::endl;
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

    if (!serverMode) {
        std::cerr << "This is the server implementation. Use -s flag to run in server mode." << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    try {
        // Create event loop
        auto loop = EventLoop::createEventLoop();
        
        // Create iperf3 server
        IPerf3Server server(loop, port, testDuration);

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