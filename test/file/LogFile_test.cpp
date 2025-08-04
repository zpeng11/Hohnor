#include "hohnor/file/LogFile.h"
#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <thread>
#include <chrono>
#include <regex>
#include <dirent.h>

using namespace Hohnor;

class LogFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        mkdir("test_logs", 0755);
        
        testBasename_ = "test_log";
        testDirectory_ = "test_logs/";
        
        // Clean up any existing test log files
        cleanupLogFiles();
    }
    
    void TearDown() override {
        // Clean up test log files
        cleanupLogFiles();
        rmdir("test_logs");
    }
    
    void cleanupLogFiles() {
        DIR* dir = opendir("test_logs");
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (strstr(entry->d_name, "test_log") != nullptr) {
                    std::string filepath = std::string("test_logs/") + entry->d_name;
                    unlink(filepath.c_str());
                }
            }
            closedir(dir);
        }
    }
    
    std::vector<std::string> getLogFiles() {
        std::vector<std::string> files;
        DIR* dir = opendir("test_logs");
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (strstr(entry->d_name, "test_log") != nullptr) {
                    files.push_back(entry->d_name);
                }
            }
            closedir(dir);
        }
        std::sort(files.begin(), files.end());
        return files;
    }
    
    std::string readFileContent(const std::string& filename) {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            return "";
        }
        std::string content((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
        return content;
    }
    
    off_t getFileSize(const std::string& filename) {
        struct stat statbuf;
        if (stat(filename.c_str(), &statbuf) == 0) {
            return statbuf.st_size;
        }
        return -1;
    }
    
    std::string testBasename_;
    std::string testDirectory_;
};

// Constructor Tests
TEST_F(LogFileTest, ConstructorWithDefaultParameters) {
    {
        LogFile logFile(testBasename_, testDirectory_);
        // Constructor should create initial log file
    }
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    EXPECT_TRUE(files[0].find(testBasename_) != std::string::npos);
    EXPECT_TRUE(files[0].find(".log") != std::string::npos);
}

TEST_F(LogFileTest, ConstructorWithCustomParameters) {
    {
        LogFile logFile(testBasename_, testDirectory_, 512, 1, 1024, 3600, Timestamp::GMT);
        // Constructor should create initial log file with custom parameters
    }
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    EXPECT_TRUE(files[0].find(testBasename_) != std::string::npos);
    EXPECT_TRUE(files[0].find(".log") != std::string::npos);
}

TEST_F(LogFileTest, ConstructorWithCurrentDirectory) {
    {
        LogFile logFile(testBasename_);
        // Should create log file in current directory
    }
    
    // Check if file was created in current directory
    DIR* dir = opendir(".");
    bool found = false;
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strstr(entry->d_name, "test_log") != nullptr && strstr(entry->d_name, ".log") != nullptr) {
                found = true;
                unlink(entry->d_name); // Clean up
                break;
            }
        }
        closedir(dir);
    }
    EXPECT_TRUE(found);
}

// Basic Append Tests
TEST_F(LogFileTest, BasicAppend) {
    LogFile logFile(testBasename_, testDirectory_);
    
    std::string testMessage = "Test log message\n";
    logFile.append(testMessage.c_str(), testMessage.length());
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    std::string filepath = testDirectory_ + files[0];
    std::string content = readFileContent(filepath);
    EXPECT_EQ(content, testMessage);
}

TEST_F(LogFileTest, MultipleAppends) {
    LogFile logFile(testBasename_, testDirectory_);
    
    std::string message1 = "First message\n";
    std::string message2 = "Second message\n";
    std::string message3 = "Third message\n";
    
    logFile.append(message1.c_str(), message1.length());
    logFile.append(message2.c_str(), message2.length());
    logFile.append(message3.c_str(), message3.length());
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    std::string filepath = testDirectory_ + files[0];
    std::string content = readFileContent(filepath);
    EXPECT_EQ(content, message1 + message2 + message3);
}

TEST_F(LogFileTest, AppendEmptyMessage) {
    LogFile logFile(testBasename_, testDirectory_);
    
    logFile.append("", 0);
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    std::string filepath = testDirectory_ + files[0];
    off_t fileSize = getFileSize(filepath);
    EXPECT_EQ(fileSize, 0);
}

// Roll File Tests
TEST_F(LogFileTest, ManualRollFile) {
    LogFile logFile(testBasename_, testDirectory_);
    
    std::string message1 = "Message in first file\n";
    logFile.append(message1.c_str(), message1.length());
    logFile.flush();
    
    // Manual roll
    logFile.rollFile();
    
    std::string message2 = "Message in second file\n";
    logFile.append(message2.c_str(), message2.length());
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 2);
    
    // Check content of both files
    std::string content1 = readFileContent(testDirectory_ + files[0]);
    std::string content2 = readFileContent(testDirectory_ + files[1]);
    
    EXPECT_EQ(content1, message1);
    EXPECT_EQ(content2, message2);
}

TEST_F(LogFileTest, AutoRollBasedOnSize) {
    // Set small roll size to trigger rolling
    off_t smallRollSize = 50;
    LogFile logFile(testBasename_, testDirectory_, 1024, 3, smallRollSize);
    
    // Write enough data to exceed roll size
    std::string longMessage = "This is a long message that should trigger rolling when accumulated\n";
    logFile.append(longMessage.c_str(), longMessage.length());
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 2); // Should have rolled to a new file
}

TEST_F(LogFileTest, AutoRollBasedOnTimeInterval) {
    // Set short roll interval (1 second) and check interval (1 write)
    LogFile logFile(testBasename_, testDirectory_, 1, 3, 65536 * 256, 1);
    
    std::string message1 = "First message\n";
    logFile.append(message1.c_str(), message1.length());
    
    // Wait for roll interval to pass
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::string message2 = "Second message after time interval\n";
    logFile.append(message2.c_str(), message2.length());
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 2); // Should have rolled due to time interval
}

// Flush Tests
TEST_F(LogFileTest, ManualFlush) {
    LogFile logFile(testBasename_, testDirectory_);
    
    std::string message = "Test message for flush\n";
    logFile.append(message.c_str(), message.length());
    
    // Before flush, content might not be written to disk yet
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    std::string filepath = testDirectory_ + files[0];
    std::string content = readFileContent(filepath);
    EXPECT_EQ(content, message);
}

TEST_F(LogFileTest, AutoFlushBasedOnTimeInterval) {
    // Set short flush interval (1 second) and check interval (1 write)
    LogFile logFile(testBasename_, testDirectory_, 1, 1, 65536 * 256, 60 * 60 * 24);
    
    std::string message = "Message for auto flush test\n";
    logFile.append(message.c_str(), message.length());
    
    // Wait for flush interval to pass
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Trigger check by appending another message
    std::string message2 = "Second message\n";
    logFile.append(message2.c_str(), message2.length());
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    std::string filepath = testDirectory_ + files[0];
    std::string content = readFileContent(filepath);
    EXPECT_EQ(content, message + message2);
}

// Filename Generation Tests
TEST_F(LogFileTest, FilenameFormatUTC) {
    LogFile logFile(testBasename_, testDirectory_, 1024, 3, 65536 * 256, 60 * 60 * 24, Timestamp::UTC);
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    // Check filename format: basename.YYYYMMDD-HHMMSS.log
    std::regex pattern(testBasename_ + R"(\.(\d{8})-(\d{6})-(\d{8})\.log)");
    EXPECT_TRUE(std::regex_match(files[0], pattern));
}

TEST_F(LogFileTest, FilenameFormatGMT) {
    LogFile logFile(testBasename_, testDirectory_, 1024, 3, 65536 * 256, 60 * 60 * 24, Timestamp::GMT);
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    // Check filename format: basename.YYYYMMDD-HHMMSS.log
    std::regex pattern(testBasename_ + R"(\.(\d{8})-(\d{6})-(\d{8})\.log)");
    EXPECT_TRUE(std::regex_match(files[0], pattern));
}

TEST_F(LogFileTest, FilenameUniqueness) {
    LogFile logFile(testBasename_, testDirectory_);
    
    // Force multiple rolls in quick succession
    for (int i = 0; i < 3; ++i) {
        logFile.rollFile();
        // Small delay to ensure different timestamps
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto files = getLogFiles();
    EXPECT_GE(files.size(), 3);
    
    // All filenames should be unique
    std::set<std::string> uniqueFiles(files.begin(), files.end());
    EXPECT_EQ(uniqueFiles.size(), files.size());
}

// Check Interval Tests
TEST_F(LogFileTest, CheckIntervalBehavior) {
    // Set checkEveryN to 3
    LogFile logFile(testBasename_, testDirectory_, 3, 1, 65536 * 256, 1);
    
    std::string message = "Test message\n";
    
    // First two appends shouldn't trigger check
    logFile.append(message.c_str(), message.length());
    logFile.append(message.c_str(), message.length());
    
    // Third append should trigger check (but no roll/flush due to timing)
    logFile.append(message.c_str(), message.length());
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
}

// Edge Cases and Error Conditions
TEST_F(LogFileTest, InvalidBasenameWithSlash) {
    // Constructor should assert if basename contains '/'
    // This test verifies the assertion behavior
    EXPECT_DEATH({
        LogFile logFile("invalid/basename", testDirectory_);
    }, "");
}

TEST_F(LogFileTest, LargeMessage) {
    LogFile logFile(testBasename_, testDirectory_);
    
    // Create a large message (larger than typical buffer)
    std::string largeMessage;
    for (int i = 0; i < 1000; ++i) {
        largeMessage += "This is a long line of text for testing large message handling.\n";
    }
    
    logFile.append(largeMessage.c_str(), largeMessage.length());
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    std::string filepath = testDirectory_ + files[0];
    std::string content = readFileContent(filepath);
    EXPECT_EQ(content, largeMessage);
}

TEST_F(LogFileTest, DirectoryWithoutTrailingSlash) {
    std::string dirWithoutSlash = "test_logs";
    LogFile logFile(testBasename_, dirWithoutSlash);
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    EXPECT_TRUE(files[0].find(testBasename_) != std::string::npos);
}

TEST_F(LogFileTest, ZeroLengthAppend) {
    LogFile logFile(testBasename_, testDirectory_);
    
    std::string message = "Before zero append\n";
    logFile.append(message.c_str(), message.length());
    logFile.append(nullptr, 0);  // Zero length append
    
    std::string message2 = "After zero append\n";
    logFile.append(message2.c_str(), message2.length());
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 1);
    
    std::string filepath = testDirectory_ + files[0];
    std::string content = readFileContent(filepath);
    EXPECT_EQ(content, message + message2);
}

// Performance and Stress Tests
TEST_F(LogFileTest, ManySmallAppends) {
    LogFile logFile(testBasename_, testDirectory_);
    
    std::string expectedContent;
    for (int i = 0; i < 1000; ++i) {
        std::string message = "Message " + std::to_string(i) + "\n";
        logFile.append(message.c_str(), message.length());
        expectedContent += message;
    }
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_GE(files.size(), 1);
    
    // Read all files and concatenate content
    std::string totalContent;
    for (const auto& file : files) {
        std::string filepath = testDirectory_ + file;
        totalContent += readFileContent(filepath);
    }
    
    EXPECT_EQ(totalContent, expectedContent);
}

TEST_F(LogFileTest, ConcurrentFileOperations) {
    // Test that LogFile handles file operations correctly
    // Note: LogFile is not thread-safe, but we test sequential operations
    LogFile logFile(testBasename_, testDirectory_);
    
    std::string message1 = "First operation\n";
    logFile.append(message1.c_str(), message1.length());
    logFile.flush();
    
    logFile.rollFile();
    
    std::string message2 = "Second operation\n";
    logFile.append(message2.c_str(), message2.length());
    logFile.flush();
    
    auto files = getLogFiles();
    EXPECT_EQ(files.size(), 2);
}