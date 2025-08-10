/**
 * iperf3-style TCP Client Benchmark using Hohnor EventLoop and TCPConnector
 * This client measures TCP throughput by sending data to a server and reporting statistics
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
#include <string>
#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <atomic>
#include <iomanip>
#include <vector>

using namespace Hohnor;

class IPerf3Client {
private:
    std::shared_ptr<EventLoop> loop_;
    std::shared_ptr<TCPConnector> connector_;
    std::shared_ptr<TCPConnection> connection_;
    std::string serverHost_;
    uint16_t serverPort_;
    bool connected_;
    bool running_;
    int testDuration_;
    int parallelStreams_;
    size_t bufferSize_;
    
    // Statistics
    std::atomic<uint64_t> bytesSent_;
    Timestamp testStartTime_;
    Timestamp lastReportTime_;
    uint64_t lastBytesSent_;
    
    // Data buffer for sending
    std::vector<char> sendBuffer_;
    
    // Statistics reporting
    static constexpr double REPORT_INTERVAL = 1.0; // Report every 1 second

public:
    IPerf3Client(std::shared_ptr<EventLoop> loop, const std::string& host, uint16_t port, 
                 int duration = 10, int parallel = 1, size_t bufSize = 128 * 1024)
        : loop_(loop), serverHost_(host), serverPort_(port),
          connected_(false), running_(false), testDuration_(duration), 
          parallelStreams_(parallel), bufferSize_(bufSize),
          bytesSent_(0), lastBytesSent_(0) {
        
        // Initialize send buffer with test data
        sendBuffer_.resize(bufferSize_);
        for (size_t i = 0; i < bufferSize_; ++i) {
            sendBuffer_[i] = static_cast<char>('A' + (i % 26));
        }
    }

    void start() {
        if (running_) {
            std::cout << "Client is already running!" << std::endl;
            return;
        }
        // Logger::setGlobalLogLevel(Logger::LogLevel::TRACE);

        try {
            // Create TCP connector
            InetAddress serverAddr(serverHost_, serverPort_);
            connector_ = std::make_shared<TCPConnector>(loop_, serverAddr);
            
            // Set up connection callbacks
            connector_->setNewConnectionCallback([this](std::shared_ptr<TCPConnection> connection) {
                LOG_DEBUG << "Reach here";
                this->handleNewConnection(connection);
            });

            connector_->setRetryConnectionCallback([this]() {
                std::cout << "Retrying connection to server..." << std::endl;
            });

            connector_->setFailedConnectionCallback([this]() {
                std::cerr << "Failed to connect to server after all retries" << std::endl;
                this->handleError();
            });

            // Configure retry behavior
            connector_->setRetries(3);
            connector_->setRetryConstantDelay(1000);

            running_ = true;
            
            std::cout << "-----------------------------------------------------------" << std::endl;
            std::cout << "Client connecting to " << serverAddr.toIpPort() << ", TCP port " << serverPort_ << std::endl;
            std::cout << "TCP window size: " << std::fixed << std::setprecision(1) << (bufferSize_ / 1024.0) << " KByte" << std::endl;
            std::cout << "-----------------------------------------------------------" << std::endl;
            
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
        
        // Print final statistics
        printFinalStats();
        
        if (connection_) {
            connection_->forceClose();
            connection_.reset();
        }
        
        if (connector_) {
            connector_->stop();
            connector_.reset();
        }
        
        std::cout << "iperf Done." << std::endl;
    }

private:
    void handleNewConnection(std::shared_ptr<TCPConnection> connection) {
        std::cout << "[  4] local " << connection->getLocalAddr().toIpPort() 
                  << " port " << connection->getLocalAddr().port()
                  << " connected to " << connection->getPeerAddr().toIpPort() 
                  << " port " << serverPort_ << std::endl;
        
        connection_ = connection;
        connected_ = true;
        
        // Set TCP options for performance
        connection_->setTCPNoDelay(true);

        LOG_DEBUG << "TCP_NODELAY set";

        // Set up callbacks for the connection
        connection_->setWriteCompleteCallback([this](TCPConnectionWeakPtr weakConn) {
            if (auto conn = weakConn.lock()) {
                this->handleWriteComplete();
            }
        });

        connection_->setCloseCallback([this]() {
            this->handleDisconnect();
        });

        connection_->setErrorCallback([this]() {
            this->handleError();
        });
        
        // Initialize test timing
        testStartTime_ = Timestamp::now();
        lastReportTime_ = testStartTime_;
        
        // Schedule periodic statistics reporting
        scheduleStatsReport();
        
        // Schedule test end
        loop_->addTimer([this]() {
            this->endTest();
        }, addTime(testStartTime_, testDuration_));
        
        // Start sending data
        sendData();
    }

    void sendData() {
        if (!running_ || !connected_) return;
        
        try {
            // Send data continuously
            connection_->write(sendBuffer_.data(), sendBuffer_.size());
            bytesSent_ += sendBuffer_.size();
            // Continue sending (write complete callback will trigger next send)
        } catch (const std::exception& e) {
            std::cerr << "Error sending data: " << e.what() << std::endl;
            handleError();
        }
    }

    void handleWriteComplete() {
        if (!running_ || !connected_) return;

        // Continue sending data
        sendData();
    }

    void scheduleStatsReport() {
        if (!running_) return;
        
        loop_->addTimer([this]() {
            this->printIntervalStats();
            this->scheduleStatsReport(); // Schedule next report
        }, addTime(Timestamp::now(), REPORT_INTERVAL));
    }

    void printIntervalStats() {
        if (!running_ || !connected_) return;
        
        Timestamp now = Timestamp::now();
        double intervalDuration = timeDifference(now, lastReportTime_);
        uint64_t currentBytes = bytesSent_.load();
        uint64_t intervalBytes = currentBytes - lastBytesSent_;
        double intervalThroughputMbps = (intervalBytes * 8.0) / (intervalDuration * 1000000.0);
        
        double totalDuration = timeDifference(now, testStartTime_);
        
        std::cout << "[  4] " 
                  << std::fixed << std::setprecision(1) 
                  << totalDuration - intervalDuration << "-" << totalDuration << " sec  "
                  << std::setw(8) << formatBytes(intervalBytes) << "  "
                  << std::setw(8) << std::setprecision(1) << intervalThroughputMbps << " Mbits/sec" << std::endl;
        
        // Update for next interval
        lastReportTime_ = now;
        lastBytesSent_ = currentBytes;
    }

    void printFinalStats() {
        if (!connected_) return;
        
        Timestamp now = Timestamp::now();
        double totalDuration = timeDifference(now, testStartTime_);
        uint64_t totalBytes = bytesSent_.load();
        double totalThroughputMbps = (totalBytes * 8.0) / (totalDuration * 1000000.0);
        
        std::cout << "- - - - - - - - - - - - - - - - - - - - - - - - -" << std::endl;
        std::cout << "[  4] " 
                  << std::fixed << std::setprecision(1) << "0.0-" << totalDuration << " sec  "
                  << std::setw(8) << formatBytes(totalBytes) << "  "
                  << std::setw(8) << std::setprecision(1) << totalThroughputMbps << " Mbits/sec                  sender" << std::endl;
        std::cout << "[  4] " 
                  << std::fixed << std::setprecision(1) << "0.0-" << totalDuration << " sec  "
                  << std::setw(8) << formatBytes(totalBytes) << "  "
                  << std::setw(8) << std::setprecision(1) << totalThroughputMbps << " Mbits/sec                  receiver" << std::endl;
        std::cout << std::endl;
        std::cout << "iperf Done." << std::endl;
    }

    void endTest() {
        std::cout << "\nTest completed after " << testDuration_ << " seconds." << std::endl;
        stop();
        loop_->endLoop();
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
    std::cout << "  -c, --client <host>   Run in client mode, connecting to <host>" << std::endl;
    std::cout << "  -p, --port <port>     Server port to connect to (default: 5201)" << std::endl;
    std::cout << "  -t, --time <sec>      Time in seconds to transmit (default: 10)" << std::endl;
    std::cout << "  -l, --length <len>    Length of buffer to read or write (default: 128K)" << std::endl;
    std::cout << "  -P, --parallel <n>    Number of parallel client streams (default: 1)" << std::endl;
    std::cout << "  -h, --help            Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program << " -c 127.0.0.1 -p 5201 -t 10" << std::endl;
}

size_t parseSize(const std::string& str) {
    size_t value = std::stoull(str);
    char suffix = str.back();
    
    switch (suffix) {
        case 'K': case 'k':
            return value * 1024;
        case 'M': case 'm':
            return value * 1024 * 1024;
        case 'G': case 'g':
            return value * 1024 * 1024 * 1024;
        default:
            return value;
    }
}

int main(int argc, char* argv[]) {
    std::string serverHost;
    uint16_t port = 5201;  // Default iperf3 port
    int testDuration = 10; // Default 10 seconds
    int parallelStreams = 1; // Default 1 stream
    size_t bufferSize = 128 * 1024; // Default 128KB
    bool clientMode = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-c" || arg == "--client") {
            if (i + 1 < argc) {
                serverHost = argv[++i];
                clientMode = true;
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
        } else if (arg == "-t" || arg == "--time") {
            if (i + 1 < argc) {
                testDuration = std::atoi(argv[++i]);
                if (testDuration <= 0) {
                    std::cerr << "Invalid test duration: " << argv[i] << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Option " << arg << " requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "-l" || arg == "--length") {
            if (i + 1 < argc) {
                try {
                    bufferSize = parseSize(argv[++i]);
                    if (bufferSize == 0) {
                        std::cerr << "Invalid buffer size: " << argv[i] << std::endl;
                        return 1;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Invalid buffer size: " << argv[i] << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Option " << arg << " requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "-P" || arg == "--parallel") {
            if (i + 1 < argc) {
                parallelStreams = std::atoi(argv[++i]);
                if (parallelStreams <= 0) {
                    std::cerr << "Invalid number of parallel streams: " << argv[i] << std::endl;
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

    if (!clientMode) {
        std::cerr << "This is the client implementation. Use -c <host> flag to run in client mode." << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    if (parallelStreams > 1) {
        std::cout << "Note: Parallel streams not yet implemented. Using single stream." << std::endl;
        parallelStreams = 1;
    }

    try {
        // Create event loop
        auto loop = EventLoop::createEventLoop();
        
        // Create iperf3 client
        IPerf3Client client(loop, serverHost, port, testDuration, parallelStreams, bufferSize);

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