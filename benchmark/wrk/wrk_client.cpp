/**
 * HTTP Client for testing wrk-compatible server using Hohnor TCPConnector
 * This client can simulate HTTP load testing similar to wrk
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/net/TCPConnector.h"
#include "hohnor/net/InetAddress.h"
#include "hohnor/net/TCPConnection.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/log/Logging.h"
#include <iostream>
#include <memory>
#include <vector>
#include <atomic>
#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

using namespace Hohnor;

struct ClientStats {
    std::atomic<uint64_t> requestsSent{0};
    std::atomic<uint64_t> responsesReceived{0};
    std::atomic<uint64_t> bytesSent{0};
    std::atomic<uint64_t> bytesReceived{0};
    std::atomic<uint64_t> errors{0};
    Timestamp startTime;
    
    ClientStats() : startTime(Timestamp::now()) {}
};

class HttpClient {
private:
    EventLoopPtr loop_;
    std::vector<TCPConnectorPtr> connectors_;
    std::vector<TCPConnectionPtr> connections_;
    std::string serverHost_;
    uint16_t serverPort_;
    int numConnections_;
    int testDuration_;
    bool running_;
    ClientStats stats_;
    
    // HTTP request template
    std::string httpRequest_;
    
    // Test control
    Timestamp testEndTime_;

public:
    HttpClient(EventLoopPtr loop, const std::string& host, uint16_t port, 
               int connections, int duration)
        : loop_(loop), serverHost_(host), serverPort_(port), 
          numConnections_(connections), testDuration_(duration), running_(false) {
        
        prepareHttpRequest();
        testEndTime_ = addTime(Timestamp::now(), testDuration_);
    }

    void start() {
        if (running_) {
            std::cout << "Client is already running!" << std::endl;
            return;
        }

        running_ = true;
        stats_.startTime = Timestamp::now();
        testEndTime_ = addTime(stats_.startTime, testDuration_);

        std::cout << "====================================================" << std::endl;
        std::cout << "HTTP Load Test Client" << std::endl;
        std::cout << "Target: http://" << serverHost_ << ":" << serverPort_ << "/" << std::endl;
        std::cout << "Connections: " << numConnections_ << std::endl;
        std::cout << "Duration: " << testDuration_ << " seconds" << std::endl;
        std::cout << "====================================================" << std::endl;

        try {
            // Create connectors and establish connections
            for (int i = 0; i < numConnections_; ++i) {
                createConnection(i);
            }

            // Schedule test end
            loop_->addTimer([this]() {
                this->endTest();
            }, testEndTime_);

            // Schedule periodic stats reporting
            scheduleStatsReport();

        } catch (const std::exception& e) {
            std::cerr << "Failed to start client: " << e.what() << std::endl;
            throw;
        }
    }

    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        // Print final statistics
        printFinalStats();
        
        // Close all connections
        for (auto& conn : connections_) {
            if (conn && !conn->isClosed()) {
                conn->forceClose();
            }
        }
        connections_.clear();
        connectors_.clear();
        
        std::cout << "HTTP Client stopped." << std::endl;
    }

private:
    void prepareHttpRequest() {
        std::ostringstream oss;
        oss << "GET / HTTP/1.1\r\n"
            << "Host: " << serverHost_ << ":" << serverPort_ << "\r\n"
            << "User-Agent: Hohnor-wrk-client/1.0\r\n"
            << "Accept: */*\r\n"
            << "Connection: keep-alive\r\n"
            << "\r\n";
        httpRequest_ = oss.str();
    }

    void createConnection(int connId) {
        try {
            InetAddress serverAddr(serverHost_, serverPort_);
            auto connector = std::make_shared<TCPConnector>(loop_, serverAddr);
            
            // Set connection callbacks
            connector->setNewConnectionCallback([this, connId](TCPConnectionPtr conn) {
                this->handleNewConnection(connId, conn);
            });
            
            connector->setRetryConnectionCallback([this, connId]() {
                std::cout << "Retrying connection " << connId << "..." << std::endl;
            });
            
            connector->setFailedConnectionCallback([this, connId]() {
                std::cerr << "Failed to establish connection " << connId << std::endl;
                stats_.errors++;
            });
            
            // Configure retry behavior
            connector->setRetries(3);  // Retry 3 times
            connector->setRetryConstantDelay(1000);  // 1 second delay
            
            connectors_.push_back(connector);
            connector->start();
            
        } catch (const std::exception& e) {
            std::cerr << "Error creating connection " << connId << ": " << e.what() << std::endl;
            stats_.errors++;
        }
    }

    void handleNewConnection(int connId, TCPConnectionPtr conn) {
        if (!conn) {
            std::cerr << "Connection " << connId << " is null" << std::endl;
            stats_.errors++;
            return;
        }

        std::cout << "Connection " << connId << " established" << std::endl;
        
        // Store connection
        if (connId < static_cast<int>(connections_.size())) {
            connections_[connId] = conn;
        } else {
            connections_.resize(connId + 1);
            connections_[connId] = conn;
        }
        
        // Set TCP options for performance
        conn->setTCPNoDelay(true);
        
        // Set up connection callbacks
        conn->setReadCompleteCallback([this, connId](TCPConnectionPtr conn) {
            this->handleHttpResponse(connId, conn);
        });
        
        conn->setCloseCallback([this, connId]() {
            this->handleConnectionClose(connId);
        });
        
        conn->setErrorCallback([this, connId]() {
            this->handleConnectionError(connId);
        });
        
        // Start sending requests
        sendHttpRequest(connId, conn);
    }

    void sendHttpRequest(int connId, TCPConnectionPtr conn) {
        if (!running_ || !conn || conn->isClosed()) {
            return;
        }
        
        // Check if test time has expired
        if (Timestamp::now() >= testEndTime_) {
            return;
        }
        
        try {
            conn->write(httpRequest_);
            stats_.requestsSent++;
            stats_.bytesSent += httpRequest_.length();
            
            // Start reading response
            conn->readUntil("\r\n\r\n"); // Read until end of HTTP headers
            
        } catch (const std::exception& e) {
            std::cerr << "Error sending request on connection " << connId << ": " << e.what() << std::endl;
            stats_.errors++;
        }
    }

    void handleHttpResponse(int connId, TCPConnectionPtr conn) {
        try {
            Buffer& readBuffer = conn->getReadBuffer();
            
            if (readBuffer.readableBytes() > 0) {
                std::string response = readBuffer.retrieveAllAsString();
                stats_.responsesReceived++;
                stats_.bytesReceived += response.length();
                
                // Parse response to check if we need to read more (for Content-Length)
                // For simplicity, we assume the response is complete
                
                // Continue sending requests if test is still running
                if (running_ && Timestamp::now() < testEndTime_) {
                    // Small delay to avoid overwhelming the server
                    loop_->addTimer([this, connId, conn]() {
                        this->sendHttpRequest(connId, conn);
                    }, addTime(Timestamp::now(), 0.001)); // 1ms delay
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling response on connection " << connId << ": " << e.what() << std::endl;
            stats_.errors++;
        }
    }

    void handleConnectionClose(int connId) {
        std::cout << "Connection " << connId << " closed" << std::endl;
        
        // Try to reconnect if test is still running
        if (running_ && Timestamp::now() < testEndTime_) {
            loop_->addTimer([this, connId]() {
                this->createConnection(connId);
            }, addTime(Timestamp::now(), 1.0)); // Reconnect after 1 second
        }
    }

    void handleConnectionError(int connId) {
        std::cerr << "Error on connection " << connId << std::endl;
        stats_.errors++;
        handleConnectionClose(connId);
    }

    void scheduleStatsReport() {
        if (!running_) return;
        
        loop_->addTimer([this]() {
            this->printIntervalStats();
            if (running_) {
                this->scheduleStatsReport(); // Schedule next report
            }
        }, addTime(Timestamp::now(), 2.0)); // Report every 2 seconds
    }

    void printIntervalStats() {
        if (!running_) return;
        
        Timestamp now = Timestamp::now();
        double elapsed = timeDifference(now, stats_.startTime);
        
        uint64_t requests = stats_.requestsSent.load();
        uint64_t responses = stats_.responsesReceived.load();
        uint64_t bytesSent = stats_.bytesSent.load();
        uint64_t bytesReceived = stats_.bytesReceived.load();
        uint64_t errors = stats_.errors.load();
        
        double requestsPerSec = requests / elapsed;
        double responsesPerSec = responses / elapsed;
        
        std::cout << "[" << std::fixed << std::setprecision(1) << elapsed << "s] "
                  << "Requests: " << requests << " (" << std::setprecision(0) << requestsPerSec << " req/s), "
                  << "Responses: " << responses << " (" << responsesPerSec << " resp/s), "
                  << "Errors: " << errors << std::endl;
    }

    void printFinalStats() {
        Timestamp now = Timestamp::now();
        double totalDuration = timeDifference(now, stats_.startTime);
        
        uint64_t finalRequests = stats_.requestsSent.load();
        uint64_t finalResponses = stats_.responsesReceived.load();
        uint64_t finalBytesSent = stats_.bytesSent.load();
        uint64_t finalBytesReceived = stats_.bytesReceived.load();
        uint64_t finalErrors = stats_.errors.load();
        
        double avgRequestsPerSec = finalRequests / totalDuration;
        double avgResponsesPerSec = finalResponses / totalDuration;
        double successRate = finalRequests > 0 ? (double)finalResponses / finalRequests * 100.0 : 0.0;
        
        std::cout << "====================================================" << std::endl;
        std::cout << "Final Client Statistics:" << std::endl;
        std::cout << "Total Duration: " << std::fixed << std::setprecision(2) << totalDuration << " seconds" << std::endl;
        std::cout << "Requests Sent: " << finalRequests << std::endl;
        std::cout << "Responses Received: " << finalResponses << std::endl;
        std::cout << "Success Rate: " << std::setprecision(1) << successRate << "%" << std::endl;
        std::cout << "Average Requests/sec: " << std::setprecision(0) << avgRequestsPerSec << std::endl;
        std::cout << "Average Responses/sec: " << avgResponsesPerSec << std::endl;
        std::cout << "Bytes Sent: " << formatBytes(finalBytesSent) << std::endl;
        std::cout << "Bytes Received: " << formatBytes(finalBytesReceived) << std::endl;
        std::cout << "Errors: " << finalErrors << std::endl;
        std::cout << "====================================================" << std::endl;
    }

    void endTest() {
        std::cout << "\nTest duration completed." << std::endl;
        stop();
        loop_->endLoop();
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
    std::cout << "  -h, --host <host>     Server hostname (default: localhost)" << std::endl;
    std::cout << "  -p, --port <port>     Server port (default: 8080)" << std::endl;
    std::cout << "  -c, --connections <n> Number of connections (default: 10)" << std::endl;
    std::cout << "  -t, --time <sec>      Test duration in seconds (default: 10)" << std::endl;
    std::cout << "  --help                Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program << " -h localhost -p 8080 -c 100 -t 30" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string host = "localhost";
    uint16_t port = 8080;
    int connections = 10;
    int duration = 10;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--host") {
            if (i + 1 < argc) {
                host = argv[++i];
            } else {
                std::cerr << "Option " << arg << " requires an argument" << std::endl;
                return 1;
            }
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
        } else if (arg == "-c" || arg == "--connections") {
            if (i + 1 < argc) {
                connections = std::atoi(argv[++i]);
                if (connections <= 0) {
                    std::cerr << "Invalid number of connections: " << argv[i] << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Option " << arg << " requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "-t" || arg == "--time") {
            if (i + 1 < argc) {
                duration = std::atoi(argv[++i]);
                if (duration <= 0) {
                    std::cerr << "Invalid test duration: " << argv[i] << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Option " << arg << " requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "--help") {
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
        auto loop = EventLoop::create();
        
        // Create HTTP client
        HttpClient client(loop, host, port, connections, duration);

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
        
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}