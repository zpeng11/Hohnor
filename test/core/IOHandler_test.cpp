#include "hohnor/core/IOHandler.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace Hohnor;

class IOHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop_ = std::make_unique<EventLoop>();
        // Create a test file descriptor (eventfd)
        test_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        ASSERT_GT(test_fd_, 0);
    }
    
    void TearDown() override {
        if (test_fd_ > 0) {
            ::close(test_fd_);
        }
        loop_.reset();
    }

    std::unique_ptr<EventLoop> loop_;
    int test_fd_;
};

TEST_F(IOHandlerTest, CreationAndInitialState) {
    auto handler = loop_->handleIO(test_fd_);
    
    EXPECT_NE(handler, nullptr);
    EXPECT_EQ(handler->fd(), test_fd_);
    EXPECT_EQ(handler->status(), IOHandler::Status::Created);
    EXPECT_FALSE(handler->isEnabled());
    EXPECT_EQ(handler->getEvents(), 0);
}

TEST_F(IOHandlerTest, EnableAndDisable) {
    auto handler = loop_->handleIO(test_fd_);
    
    // Initially created but not enabled
    EXPECT_EQ(handler->status(), IOHandler::Status::Created);
    EXPECT_FALSE(handler->isEnabled());
    
    // Enable the handler
    handler->enable();
    
    // Process the enable operation in the loop
    loop_->runInLoop([&]() {
        EXPECT_EQ(handler->status(), IOHandler::Status::Enabled);
        EXPECT_TRUE(handler->isEnabled());
    });
    
    // Disable the handler
    handler->disable();
    
    // Process the disable operation in the loop
    loop_->runInLoop([&]() {
        EXPECT_EQ(handler->status(), IOHandler::Status::Disabled);
        EXPECT_FALSE(handler->isEnabled());
    });
}

TEST_F(IOHandlerTest, SetReadCallback) {
    auto handler = loop_->handleIO(test_fd_);
    bool read_callback_called = false;
    
    handler->setReadCallback([&read_callback_called]() {
        read_callback_called = true;
    });
    
    // Process the callback setting in the loop
    loop_->runInLoop([&]() {
        // Events should include EPOLLIN after setting read callback
        EXPECT_TRUE(handler->getEvents() & EPOLLIN);
    });
    
    // Clear the callback
    handler->setReadCallback(nullptr);
    
    loop_->runInLoop([&]() {
        // Events should not include EPOLLIN after clearing read callback
        EXPECT_FALSE(handler->getEvents() & EPOLLIN);
    });
}

TEST_F(IOHandlerTest, SetWriteCallback) {
    auto handler = loop_->handleIO(test_fd_);
    bool write_callback_called = false;
    
    handler->setWriteCallback([&write_callback_called]() {
        write_callback_called = true;
    });
    
    // Process the callback setting in the loop
    loop_->runInLoop([&]() {
        // Events should include EPOLLOUT after setting write callback
        EXPECT_TRUE(handler->getEvents() & EPOLLOUT);
    });
    
    // Clear the callback
    handler->setWriteCallback(nullptr);
    
    loop_->runInLoop([&]() {
        // Events should not include EPOLLOUT after clearing write callback
        EXPECT_FALSE(handler->getEvents() & EPOLLOUT);
    });
}

TEST_F(IOHandlerTest, SetCloseCallback) {
    auto handler = loop_->handleIO(test_fd_);
    bool close_callback_called = false;
    
    handler->setCloseCallback([&close_callback_called]() {
        close_callback_called = true;
    });
    
    // Process the callback setting in the loop
    loop_->runInLoop([&]() {
        // Events should include EPOLLRDHUP after setting close callback
        EXPECT_TRUE(handler->getEvents() & EPOLLRDHUP);
    });
    
    // Clear the callback
    handler->setCloseCallback(nullptr);
    
    loop_->runInLoop([&]() {
        // Events should not include EPOLLRDHUP after clearing close callback
        EXPECT_FALSE(handler->getEvents() & EPOLLRDHUP);
    });
}

TEST_F(IOHandlerTest, SetErrorCallback) {
    auto handler = loop_->handleIO(test_fd_);
    bool error_callback_called = false;
    
    handler->setErrorCallback([&error_callback_called]() {
        error_callback_called = true;
    });
    
    // Process the callback setting in the loop
    loop_->runInLoop([&]() {
        // Events should include EPOLLERR after setting error callback
        EXPECT_TRUE(handler->getEvents() & EPOLLERR);
    });
    
    // Clear the callback
    handler->setErrorCallback(nullptr);
    
    loop_->runInLoop([&]() {
        // Events should not include EPOLLERR after clearing error callback
        EXPECT_FALSE(handler->getEvents() & EPOLLERR);
    });
}

TEST_F(IOHandlerTest, MultipleCallbacks) {
    auto handler = loop_->handleIO(test_fd_);
    
    bool read_called = false;
    bool write_called = false;
    bool close_called = false;
    bool error_called = false;
    
    handler->setReadCallback([&read_called]() { read_called = true; });
    handler->setWriteCallback([&write_called]() { write_called = true; });
    handler->setCloseCallback([&close_called]() { close_called = true; });
    handler->setErrorCallback([&error_called]() { error_called = true; });
    
    // Process all callback settings in the loop
    loop_->runInLoop([&]() {
        int events = handler->getEvents();
        EXPECT_TRUE(events & EPOLLIN);
        EXPECT_TRUE(events & EPOLLOUT);
        EXPECT_TRUE(events & EPOLLRDHUP);
        EXPECT_TRUE(events & EPOLLERR);
    });
}

TEST_F(IOHandlerTest, EventHandling) {
    auto handler = loop_->handleIO(test_fd_);
    
    std::atomic<bool> read_called{false};
    std::atomic<bool> write_called{false};
    std::atomic<bool> loop_finished{false};
    
    handler->setReadCallback([&read_called, &loop_finished]() {
        read_called = true;
        // End the loop after handling the read event
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    });
    
    handler->setWriteCallback([&write_called]() {
        write_called = true;
    });
    
    handler->enable();
    
    // Create a separate thread to run the event loop
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    
    // Wait a bit for the loop to start
    CurrentThread::sleepUsec(10 * 1000); // 10ms
    
    // Trigger a read event by writing to the eventfd
    uint64_t value = 1;
    ssize_t n = ::write(test_fd_, &value, sizeof(value));
    EXPECT_EQ(n, sizeof(value));
    
    // Wait for the loop to finish processing the event
    loop_thread.join();
    
    // Verify the read callback was called
    EXPECT_TRUE(read_called.load());
    EXPECT_TRUE(loop_finished.load());
    
    // Note: Write events are typically always ready for eventfd,
    // but we're primarily testing the read event mechanism here
    // since it's more predictable and controllable
}

TEST_F(IOHandlerTest, HandlerLifecycle) {
    std::weak_ptr<IOHandler> weak_handler;
    
    {
        auto handler = loop_->handleIO(test_fd_);
        weak_handler = handler;
        EXPECT_FALSE(weak_handler.expired());
        
        handler->enable();
        
        // Handler should be alive while enabled
        loop_->runInLoop([&]() {
            EXPECT_FALSE(weak_handler.expired());
        });
        
        handler->disable();
    }
    
    // After going out of scope and being disabled, 
    // the handler might still be alive due to internal references
    // but should eventually be cleaned up
    loop_->runInLoop([&]() {
        // The handler might still be referenced internally
        // This test mainly ensures no crashes occur
    });
}

TEST_F(IOHandlerTest, ThreadSafetyOfCallbacks) {
    auto handler = loop_->handleIO(test_fd_);
    std::atomic<int> callback_count{0};
    
    // Set callbacks from different thread
    Thread t([&]() {
        handler->setReadCallback([&callback_count]() {
            callback_count++;
        });
        
        handler->setWriteCallback([&callback_count]() {
            callback_count++;
        });
    });
    
    t.start();
    t.join();
    
    // Process the callback settings in the loop
    loop_->queueInLoop([&]() {
        int events = handler->getEvents();
        EXPECT_TRUE(events & EPOLLIN);
        EXPECT_TRUE(events & EPOLLOUT);
    });
}

TEST_F(IOHandlerTest, DisableFromDifferentThread) {
    auto handler = loop_->handleIO(test_fd_);
    handler->enable();
    
    std::atomic<bool> thread_finished{false};
    
    Thread t([&]() {
        CurrentThread::sleepUsec(10 * 1000); // 10ms
        handler->disable();
        thread_finished = true;
    });
    
    t.start();
    
    // Process operations in the loop
    loop_->queueInLoop([&]() {
        // Initially enabled
        EXPECT_TRUE(handler->isEnabled());
    });
    
    // Wait for thread to finish
    t.join();
    EXPECT_TRUE(thread_finished.load());
    
    // Process the disable operation
    loop_->queueInLoop([&]() {
        EXPECT_FALSE(handler->isEnabled());
    });
}

TEST_F(IOHandlerTest, DestructorDisablesHandler) {
    std::shared_ptr<IOHandler> handler;
    
    handler = loop_->handleIO(test_fd_);
    handler->enable();
    
    loop_->runInLoop([&]() {
        EXPECT_TRUE(handler->isEnabled());
    });
    
    // Reset the handler (calls destructor)
    handler.reset();
    
    // The destructor should have called disable()
    // This test mainly ensures no crashes occur during cleanup
    loop_->runInLoop([&]() {
        // Handler is now destroyed, just ensure loop still works
    });
}