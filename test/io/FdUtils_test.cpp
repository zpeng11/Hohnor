#include "hohnor/io/FdUtils.h"
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

using namespace Hohnor;
using namespace Hohnor::FdUtils;

class FdUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary file for testing
        testFileName_ = "/tmp/fdutils_test_file.txt";
        testFd_ = open(testFileName_.c_str(), O_CREAT | O_RDWR, 0644);
        ASSERT_NE(testFd_, -1) << "Failed to create test file";
    }
    
    void TearDown() override {
        // Clean up
        if (testFd_ != -1) {
            ::close(testFd_);
        }
        unlink(testFileName_.c_str());
    }
    
    std::string testFileName_;
    int testFd_;
};

// Test FdUtils::close function
TEST_F(FdUtilsTest, CloseValidFd) {
    int fd = dup(testFd_);  // Create a duplicate fd for testing
    ASSERT_NE(fd, -1);
    
    // Close using FdUtils::close
    FdUtils::close(fd);
    
    // Verify fd is closed by trying to write to it
    char buffer[] = "test";
    int result = write(fd, buffer, sizeof(buffer));
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, EBADF);
}

TEST_F(FdUtilsTest, CloseInvalidFd) {
    // Test closing an invalid fd - should not crash
    EXPECT_NO_THROW(FdUtils::close(-1));
    EXPECT_NO_THROW(FdUtils::close(9999));
}

// Test FdUtils::setNonBlocking function
TEST_F(FdUtilsTest, SetNonBlockingTrue) {
    int oldFlags = setNonBlocking(testFd_, true);
    EXPECT_NE(oldFlags, -1);
    
    // Verify the fd is now non-blocking
    int currentFlags = fcntl(testFd_, F_GETFL);
    EXPECT_NE(currentFlags, -1);
    EXPECT_TRUE(currentFlags & O_NONBLOCK);
}

TEST_F(FdUtilsTest, SetNonBlockingFalse) {
    // First set to non-blocking
    setNonBlocking(testFd_, true);
    
    // Then set back to blocking
    int oldFlags = setNonBlocking(testFd_, false);
    EXPECT_NE(oldFlags, -1);
    EXPECT_TRUE(oldFlags & O_NONBLOCK);  // Old flags should have had O_NONBLOCK
    
    // Verify the fd is now blocking
    int currentFlags = fcntl(testFd_, F_GETFL);
    EXPECT_NE(currentFlags, -1);
    EXPECT_FALSE(currentFlags & O_NONBLOCK);
}

TEST_F(FdUtilsTest, SetNonBlockingInvalidFd) {
    int result = setNonBlocking(-1, true);
    EXPECT_EQ(result, -1);
}

TEST_F(FdUtilsTest, SetNonBlockingReturnsPreviousFlags) {
    // Get initial flags
    int initialFlags = fcntl(testFd_, F_GETFL);
    ASSERT_NE(initialFlags, -1);
    
    // Set non-blocking and check return value
    int returnedFlags = setNonBlocking(testFd_, true);
    EXPECT_EQ(returnedFlags, initialFlags);
    
    // Set back to blocking and check return value
    int returnedFlags2 = setNonBlocking(testFd_, false);
    EXPECT_TRUE(returnedFlags2 & O_NONBLOCK);
}

// Test FdUtils::setCloseOnExec function
TEST_F(FdUtilsTest, SetCloseOnExecTrue) {
    int oldFlags = setCloseOnExec(testFd_, true);
    EXPECT_NE(oldFlags, -1);
    
    // Verify the fd has FD_CLOEXEC set
    int currentFlags = fcntl(testFd_, F_GETFD);
    EXPECT_NE(currentFlags, -1);
    EXPECT_TRUE(currentFlags & FD_CLOEXEC);
}

TEST_F(FdUtilsTest, SetCloseOnExecFalse) {
    // First set close-on-exec
    setCloseOnExec(testFd_, true);
    
    // Then unset it
    int oldFlags = setCloseOnExec(testFd_, false);
    EXPECT_NE(oldFlags, -1);
    EXPECT_TRUE(oldFlags & FD_CLOEXEC);  // Old flags should have had FD_CLOEXEC
    
    // Verify the fd no longer has FD_CLOEXEC
    int currentFlags = fcntl(testFd_, F_GETFD);
    EXPECT_NE(currentFlags, -1);
    EXPECT_FALSE(currentFlags & FD_CLOEXEC);
}

TEST_F(FdUtilsTest, SetCloseOnExecInvalidFd) {
    int result = setCloseOnExec(-1, true);
    EXPECT_EQ(result, -1);
}

TEST_F(FdUtilsTest, SetCloseOnExecReturnsPreviousFlags) {
    // Get initial flags
    int initialFlags = fcntl(testFd_, F_GETFD);
    ASSERT_NE(initialFlags, -1);
    
    // Set close-on-exec and check return value
    int returnedFlags = setCloseOnExec(testFd_, true);
    EXPECT_EQ(returnedFlags, initialFlags);
    
    // Unset close-on-exec and check return value
    int returnedFlags2 = setCloseOnExec(testFd_, false);
    EXPECT_TRUE(returnedFlags2 & FD_CLOEXEC);
}


// FdGuard Tests
class FdGuardTest : public ::testing::Test {
protected:
    void SetUp() override {
        testFileName_ = "/tmp/fdguard_test_file.txt";
    }
    
    void TearDown() override {
        unlink(testFileName_.c_str());
    }
    
    std::string testFileName_;
};

TEST_F(FdGuardTest, ConstructorWithValidFd) {
    int fd = open(testFileName_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_NE(fd, -1);
    
    {
        FdGuard guard(fd);
        EXPECT_EQ(guard.fd(), fd);
    }
    
    // After guard goes out of scope, fd should be closed
    char buffer[] = "test";
    int result = write(fd, buffer, sizeof(buffer));
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, EBADF);
}

TEST_F(FdGuardTest, ConstructorWithInvalidFd) {
    // Should not crash with invalid fd
    EXPECT_NO_THROW({
        FdGuard guard(-1);
        EXPECT_EQ(guard.fd(), -1);
    });
}

TEST_F(FdGuardTest, DefaultConstructor) {
    FdGuard guard;
    EXPECT_EQ(guard.fd(), -1);
}

TEST_F(FdGuardTest, SetFdWithValidFd) {
    int fd = open(testFileName_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_NE(fd, -1);
    
    FdGuard guard;
    guard.setFd(fd);
    EXPECT_EQ(guard.fd(), fd);
    
    // Let guard destructor close the fd
}

TEST_F(FdGuardTest, SetFdWithInvalidFd) {
    FdGuard guard;
    
    // Setting invalid fd should trigger assertion/check
    // This test verifies the check exists but may cause program termination
    // In a real scenario, you might want to handle this more gracefully
    EXPECT_DEATH(guard.setFd(-1), ".*");
}

TEST_F(FdGuardTest, RAIIBehavior) {
    int fd = open(testFileName_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_NE(fd, -1);
    
    // Verify fd is valid before guard
    char buffer[] = "test";
    int result = write(fd, buffer, sizeof(buffer));
    EXPECT_NE(result, -1);
    
    {
        FdGuard guard(fd);
        // fd should still be valid inside scope
        result = write(fd, buffer, sizeof(buffer));
        EXPECT_NE(result, -1);
    }
    
    // After guard destructor, fd should be closed
    result = write(fd, buffer, sizeof(buffer));
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, EBADF);
}

TEST_F(FdGuardTest, NonCopyable) {
    // FdGuard should inherit from NonCopyable
    // This is a compile-time test - if it compiles, the test passes
    static_assert(std::is_base_of<NonCopyable, FdGuard>::value, "Epoll must derive from FdGuard");
}

// Integration tests
TEST_F(FdUtilsTest, CombinedFlagsTest) {
    // Test setting both non-blocking and close-on-exec
    int oldNonBlockFlags = setNonBlocking(testFd_, true);
    int oldCloseExecFlags = setCloseOnExec(testFd_, true);
    
    EXPECT_NE(oldNonBlockFlags, -1);
    EXPECT_NE(oldCloseExecFlags, -1);
    
    // Verify both flags are set
    int fileFlags = fcntl(testFd_, F_GETFL);
    int fdFlags = fcntl(testFd_, F_GETFD);
    
    EXPECT_TRUE(fileFlags & O_NONBLOCK);
    EXPECT_TRUE(fdFlags & FD_CLOEXEC);
}

TEST_F(FdUtilsTest, PipeTest) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    
    // Test with pipe fds
    EXPECT_NE(setNonBlocking(pipefd[0], true), -1);
    EXPECT_NE(setNonBlocking(pipefd[1], true), -1);
    EXPECT_NE(setCloseOnExec(pipefd[0], true), -1);
    EXPECT_NE(setCloseOnExec(pipefd[1], true), -1);
    
    FdUtils::close(pipefd[0]);
    FdUtils::close(pipefd[1]);
}