#include "hohnor/io/Epoll.h"
#include "hohnor/io/FdUtils.h"
#include <gtest/gtest.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <thread>
#include <chrono>

using namespace Hohnor;

class EpollTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a pipe for testing
        ASSERT_EQ(pipe(pipefd_), 0);
        
        // Create a socket pair for testing
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socketfd_), 0);
        
        // Create temporary file for testing
        testFileName_ = "/tmp/epoll_test_file.txt";
        testFileFd_ = open(testFileName_.c_str(), O_CREAT | O_RDWR, 0644);
        ASSERT_NE(testFileFd_, -1);
    }
    
    void TearDown() override {
        // Clean up
        if (pipefd_[0] != -1) ::close(pipefd_[0]);
        if (pipefd_[1] != -1) ::close(pipefd_[1]);
        if (socketfd_[0] != -1) ::close(socketfd_[0]);
        if (socketfd_[1] != -1) ::close(socketfd_[1]);
        if (testFileFd_ != -1) ::close(testFileFd_);
        unlink(testFileName_.c_str());
    }
    
    int pipefd_[2];
    int socketfd_[2];
    int testFileFd_;
    std::string testFileName_;
};

// Test Epoll constructor and destructor
TEST_F(EpollTest, ConstructorDefault) {
    Epoll epoll;
    EXPECT_NE(epoll.fd(), -1);
}

TEST_F(EpollTest, ConstructorWithMaxEvents) {
    Epoll epoll(512);
    EXPECT_NE(epoll.fd(), -1);
}

TEST_F(EpollTest, ConstructorWithCloseOnExec) {
    Epoll epoll(1024, true);
    EXPECT_NE(epoll.fd(), -1);
    
    // Verify close-on-exec is set
    int flags = fcntl(epoll.fd(), F_GETFD);
    EXPECT_NE(flags, -1);
    EXPECT_TRUE(flags & FD_CLOEXEC);
}

TEST_F(EpollTest, ConstructorWithoutCloseOnExec) {
    Epoll epoll(1024, false);
    EXPECT_NE(epoll.fd(), -1);
    
    // Verify close-on-exec is not set
    int flags = fcntl(epoll.fd(), F_GETFD);
    EXPECT_NE(flags, -1);
    EXPECT_FALSE(flags & FD_CLOEXEC);
}

// Test epoll_ctl interface
TEST_F(EpollTest, CtlAddEvent) {
    Epoll epoll;
    
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = pipefd_[0];
    
    int result = epoll.ctl(EPOLL_CTL_ADD, pipefd_[0], &event);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, CtlModifyEvent) {
    Epoll epoll;
    
    // First add the fd
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = pipefd_[0];
    ASSERT_EQ(epoll.ctl(EPOLL_CTL_ADD, pipefd_[0], &event), 0);
    
    // Then modify it
    event.events = EPOLLIN | EPOLLOUT;
    int result = epoll.ctl(EPOLL_CTL_MOD, pipefd_[0], &event);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, CtlDeleteEvent) {
    Epoll epoll;
    
    // First add the fd
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = pipefd_[0];
    ASSERT_EQ(epoll.ctl(EPOLL_CTL_ADD, pipefd_[0], &event), 0);
    
    // Then delete it
    int result = epoll.ctl(EPOLL_CTL_DEL, pipefd_[0], nullptr);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, CtlInvalidFd) {
    Epoll epoll;
    
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = -1;
    
    int result = epoll.ctl(EPOLL_CTL_ADD, -1, &event);
    EXPECT_EQ(result, -1);
}

// Test add method
TEST_F(EpollTest, AddBasic) {
    Epoll epoll;
    
    int result = epoll.add(pipefd_[0], EPOLLIN);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, AddWithPointer) {
    Epoll epoll;
    
    int data = 42;
    int result = epoll.add(pipefd_[0], EPOLLIN, &data);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, AddWithEdgeTrigger) {
    Epoll epoll;
    
    int result = epoll.add(pipefd_[0], EPOLLIN | EPOLLET, nullptr);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, AddMultipleFds) {
    Epoll epoll;
    
    EXPECT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    EXPECT_EQ(epoll.add(pipefd_[1], EPOLLOUT), 0);
    EXPECT_EQ(epoll.add(socketfd_[0], EPOLLIN), 0);
}

TEST_F(EpollTest, AddInvalidFd) {
    Epoll epoll;
    
    int result = epoll.add(-1, EPOLLIN);
    EXPECT_EQ(result, -1);
}

TEST_F(EpollTest, AddDuplicateFd) {
    Epoll epoll;
    
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    
    // Adding the same fd again should fail
    int result = epoll.add(pipefd_[0], EPOLLIN);
    EXPECT_EQ(result, -1);
}

// Test modify method
TEST_F(EpollTest, ModifyBasic) {
    Epoll epoll;
    
    // First add the fd
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    
    // Then modify it
    int result = epoll.modify(pipefd_[0], EPOLLIN | EPOLLOUT);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, ModifyWithPointer) {
    Epoll epoll;
    
    // First add the fd
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    
    // Then modify with pointer
    int data = 123;
    int result = epoll.modify(pipefd_[0], EPOLLIN, &data);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, ModifyNonExistentFd) {
    Epoll epoll;
    
    // Try to modify fd that wasn't added
    int result = epoll.modify(pipefd_[0], EPOLLIN);
    EXPECT_EQ(result, -1);
}

// Test remove method
TEST_F(EpollTest, RemoveBasic) {
    Epoll epoll;
    
    // First add the fd
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    
    // Then remove it
    int result = epoll.remove(pipefd_[0]);
    EXPECT_EQ(result, 0);
}

TEST_F(EpollTest, RemoveNonExistentFd) {
    Epoll epoll;
    
    // Try to remove fd that wasn't added
    int result = epoll.remove(pipefd_[0]);
    EXPECT_EQ(result, -1);
}

TEST_F(EpollTest, RemoveInvalidFd) {
    Epoll epoll;
    
    int result = epoll.remove(-1);
    EXPECT_EQ(result, -1);
}

// Test wait method
TEST_F(EpollTest, WaitTimeout) {
    Epoll epoll;
    
    // Add a fd but don't trigger any events
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    
    auto start = std::chrono::steady_clock::now();
    auto iter = epoll.wait(100);  // 100ms timeout
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_GE(duration.count(), 90);  // Should wait at least 90ms
    EXPECT_LE(duration.count(), 200); // But not more than 200ms
    
    EXPECT_EQ(iter.size(), 0);
    EXPECT_FALSE(iter.hasNext());
}

TEST_F(EpollTest, WaitWithEvent) {
    Epoll epoll;
    
    // Add pipe read end
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    
    // Write to pipe to trigger event
    char data = 'x';
    ASSERT_EQ(write(pipefd_[1], &data, 1), 1);
    
    auto iter = epoll.wait(1000);  // 1 second timeout
    
    EXPECT_EQ(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
    
    auto& event = iter.next();
    EXPECT_TRUE(event.events & EPOLLIN);
    EXPECT_EQ(event.data.fd, pipefd_[0]);
    
    EXPECT_FALSE(iter.hasNext());
}

TEST_F(EpollTest, WaitWithMultipleEvents) {
    Epoll epoll;
    
    // Add both pipe fds
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    ASSERT_EQ(epoll.add(pipefd_[1], EPOLLOUT), 0);
    
    auto iter = epoll.wait(1000);
    
    // Should have at least one event (pipe write end should be ready for writing)
    EXPECT_GE(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
    
    // Process all events
    int eventCount = 0;
    while (iter.hasNext()) {
        auto& event = iter.next();
        eventCount++;
        
        if (event.data.fd == pipefd_[1]) {
            EXPECT_TRUE(event.events & EPOLLOUT);
        }
    }
    
    EXPECT_EQ(eventCount, iter.size());
}

TEST_F(EpollTest, WaitNoTimeout) {
    Epoll epoll;
    
    // Add pipe write end (should be immediately ready)
    ASSERT_EQ(epoll.add(pipefd_[1], EPOLLOUT), 0);
    
    auto iter = epoll.wait();  // No timeout
    
    EXPECT_EQ(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
    
    auto& event = iter.next();
    EXPECT_TRUE(event.events & EPOLLOUT);
    EXPECT_EQ(event.data.fd, pipefd_[1]);
}

// Test Iterator functionality
TEST_F(EpollTest, IteratorBasic) {
    Epoll epoll;
    
    // Add pipe write end
    ASSERT_EQ(epoll.add(pipefd_[1], EPOLLOUT), 0);
    
    auto iter = epoll.wait(1000);
    
    EXPECT_EQ(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
    
    // Get the event
    auto& event = iter.next();
    EXPECT_TRUE(event.events & EPOLLOUT);
    
    // Should be no more events
    EXPECT_FALSE(iter.hasNext());
}

TEST_F(EpollTest, IteratorMultipleEvents) {
    Epoll epoll;
    
    // Add multiple fds
    ASSERT_EQ(epoll.add(pipefd_[1], EPOLLOUT), 0);
    ASSERT_EQ(epoll.add(socketfd_[0], EPOLLOUT), 0);
    
    auto iter = epoll.wait(1000);
    
    EXPECT_GE(iter.size(), 1);
    
    int count = 0;
    while (iter.hasNext()) {
        auto& event = iter.next();
        count++;
        EXPECT_TRUE(event.events & EPOLLOUT);
    }
    
    EXPECT_EQ(count, iter.size());
}

TEST_F(EpollTest, IteratorEmpty) {
    Epoll epoll;
    
    // Add fd but don't trigger events
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    
    auto iter = epoll.wait(10);  // Short timeout
    
    EXPECT_EQ(iter.size(), 0);
    EXPECT_FALSE(iter.hasNext());
}

// Test edge cases and error conditions
TEST_F(EpollTest, EdgeTriggerMode) {
    Epoll epoll;
    
    // Add with edge trigger
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN | EPOLLET, nullptr), 0);
    
    // Write data to trigger event
    char data = 'x';
    ASSERT_EQ(write(pipefd_[1], &data, 1), 1);
    
    auto iter = epoll.wait(1000);
    EXPECT_EQ(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
    
    auto& event = iter.next();
    EXPECT_TRUE(event.events & EPOLLIN);
    // EXPECT_TRUE(event.events & EPOLLET);  // Edge trigger flag should be set
}

TEST_F(EpollTest, LevelTriggerMode) {
    Epoll epoll;
    
    // Add without edge trigger (level trigger by default)
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN, nullptr), 0);
    
    // Write data to trigger event
    char data = 'x';
    ASSERT_EQ(write(pipefd_[1], &data, 1), 1);
    
    auto iter = epoll.wait(1000);
    EXPECT_EQ(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
    
    auto& event = iter.next();
    EXPECT_TRUE(event.events & EPOLLIN);
    // EXPECT_FALSE(event.events & EPOLLET);  // Edge trigger flag should not be set
}

TEST_F(EpollTest, DataPointerStorage) {
    Epoll epoll;
    
    int testData = 12345;
    ASSERT_EQ(epoll.add(pipefd_[1], EPOLLOUT, &testData), 0);
    
    auto iter = epoll.wait(1000);
    EXPECT_EQ(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
    
    auto& event = iter.next();
    EXPECT_EQ(event.data.ptr, &testData);
    EXPECT_EQ(*static_cast<int*>(event.data.ptr), 12345);
}

TEST_F(EpollTest, DataFdStorage) {
    Epoll epoll;
    
    // When no pointer is provided, fd should be stored in data.fd
    ASSERT_EQ(epoll.add(pipefd_[1], EPOLLOUT), 0);
    
    auto iter = epoll.wait(1000);
    EXPECT_EQ(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
    
    auto& event = iter.next();
    EXPECT_EQ(event.data.fd, pipefd_[1]);
}

// Test inheritance from FdGuard
TEST_F(EpollTest, InheritsFromFdGuard) {
    // Epoll should inherit from FdGuard
    static_assert(std::is_base_of<FdGuard, Epoll>::value, "Epoll must derive from FdGuard");
}

TEST_F(EpollTest, FdGuardFunctionality) {
    Epoll epoll;
    int epollFd = epoll.fd();
    
    EXPECT_NE(epollFd, -1);
    
    // The fd should be valid
    int flags = fcntl(epollFd, F_GETFL);
    EXPECT_NE(flags, -1);
}

// Test automatic non-blocking setting
TEST_F(EpollTest, AutomaticNonBlocking) {
    Epoll epoll;
    
    // When adding a fd, it should automatically be set to non-blocking
    int originalFlags = fcntl(pipefd_[0], F_GETFL);
    ASSERT_NE(originalFlags, -1);
    
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    
    int newFlags = fcntl(pipefd_[0], F_GETFL);
    EXPECT_NE(newFlags, -1);
    EXPECT_TRUE(newFlags & O_NONBLOCK);
}

// Test complex scenarios
TEST_F(EpollTest, ComplexScenario) {
    Epoll epoll;
    
    // Add multiple fds with different events
    ASSERT_EQ(epoll.add(pipefd_[0], EPOLLIN), 0);
    ASSERT_EQ(epoll.add(pipefd_[1], EPOLLOUT), 0);
    ASSERT_EQ(epoll.add(socketfd_[0], EPOLLIN), 0);
    
    // Trigger some events
    char data = 'x';
    ASSERT_EQ(write(pipefd_[1], &data, 1), 1);
    ASSERT_EQ(write(socketfd_[1], &data, 1), 1);
    
    auto iter = epoll.wait(1000);
    
    // Should have multiple events
    EXPECT_GE(iter.size(), 2);
    
    bool foundPipeRead = false;
    bool foundPipeWrite = false;
    bool foundSocketRead = false;
    
    while (iter.hasNext()) {
        auto& event = iter.next();
        
        if (event.data.fd == pipefd_[0] && (event.events & EPOLLIN)) {
            foundPipeRead = true;
        } else if (event.data.fd == pipefd_[1] && (event.events & EPOLLOUT)) {
            foundPipeWrite = true;
        } else if (event.data.fd == socketfd_[0] && (event.events & EPOLLIN)) {
            foundSocketRead = true;
        }
    }
    
    EXPECT_TRUE(foundPipeRead);
    EXPECT_TRUE(foundPipeWrite);
    EXPECT_TRUE(foundSocketRead);
}

// Test error handling in wait
TEST_F(EpollTest, WaitWithSignalMask) {
    Epoll epoll;
    
    ASSERT_EQ(epoll.add(pipefd_[1], EPOLLOUT), 0);
    
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGUSR1);
    
    auto iter = epoll.wait(1000, &sigmask);
    
    EXPECT_EQ(iter.size(), 1);
    EXPECT_TRUE(iter.hasNext());
}