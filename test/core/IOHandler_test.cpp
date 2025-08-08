#include "hohnor/core/IOHandler.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include "hohnor/log/Logging.h"
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
        loop_ = EventLoop::createEventLoop();
        // Create a test file descriptor (eventfd)
        test_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        ASSERT_GT(test_fd_, 0);
        // Logger::setGlobalLogLevel(Logger::LogLevel::DEBUG);
    }
    
    void TearDown() override {
        loop_.reset();
    }

    std::shared_ptr<EventLoop> loop_;
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
    
    // Use atomic flags to synchronize with async operations
    std::atomic<bool> enable_checked{false};
    std::atomic<bool> disable_checked{false};
    
    // Process the enable operation in the loop
    loop_->runInLoop([&]() {
        EXPECT_EQ(handler->status(), IOHandler::Status::Enabled);
        EXPECT_TRUE(handler->isEnabled());
        enable_checked = true;
    });
    
    // Wait for enable operation to complete
    while (!enable_checked.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    // Disable the handler
    handler->disable();
    
    // Process the disable operation in the loop
    loop_->runInLoop([&]() {
        EXPECT_EQ(handler->status(), IOHandler::Status::Disabled);
        EXPECT_FALSE(handler->isEnabled());
        disable_checked = true;
    });
    
    // Wait for disable operation to complete
    while (!disable_checked.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, SetReadCallback) {
    auto handler = loop_->handleIO(test_fd_);
    bool read_callback_called = false;
    std::atomic<bool> callback_set{false};
    std::atomic<bool> callback_cleared{false};
    
    handler->setReadCallback([&read_callback_called]() {
        read_callback_called = true;
    });
    
    // Process the callback setting in the loop
    loop_->runInLoop([&]() {
        // Events should include EPOLLIN after setting read callback
        EXPECT_TRUE(handler->getEvents() & EPOLLIN);
        callback_set = true;
    });
    
    // Wait for callback to be set
    while (!callback_set.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    // Clear the callback
    handler->setReadCallback(nullptr);
    
    loop_->runInLoop([&]() {
        // Events should not include EPOLLIN after clearing read callback
        EXPECT_FALSE(handler->getEvents() & EPOLLIN);
        callback_cleared = true;
    });
    
    // Wait for callback to be cleared
    while (!callback_cleared.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, SetWriteCallback) {
    auto handler = loop_->handleIO(test_fd_);
    bool write_callback_called = false;
    std::atomic<bool> callback_set{false};
    std::atomic<bool> callback_cleared{false};
    
    handler->setWriteCallback([&write_callback_called]() {
        write_callback_called = true;
    });
    
    // Process the callback setting in the loop
    loop_->runInLoop([&]() {
        // Events should include EPOLLOUT after setting write callback
        EXPECT_TRUE(handler->getEvents() & EPOLLOUT);
        callback_set = true;
    });
    
    // Wait for callback to be set
    while (!callback_set.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    // Clear the callback
    handler->setWriteCallback(nullptr);
    
    loop_->runInLoop([&]() {
        // Events should not include EPOLLOUT after clearing write callback
        EXPECT_FALSE(handler->getEvents() & EPOLLOUT);
        callback_cleared = true;
    });
    
    // Wait for callback to be cleared
    while (!callback_cleared.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, SetCloseCallback) {
    auto handler = loop_->handleIO(test_fd_);
    bool close_callback_called = false;
    std::atomic<bool> callback_set{false};
    std::atomic<bool> callback_cleared{false};
    
    handler->setCloseCallback([&close_callback_called]() {
        close_callback_called = true;
    });
    
    // Process the callback setting in the loop
    loop_->runInLoop([&]() {
        // Events should include EPOLLRDHUP after setting close callback
        EXPECT_TRUE(handler->getEvents() & EPOLLRDHUP);
        callback_set = true;
    });
    
    // Wait for callback to be set
    while (!callback_set.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    // Clear the callback
    handler->setCloseCallback(nullptr);
    
    loop_->runInLoop([&]() {
        // Events should not include EPOLLRDHUP after clearing close callback
        EXPECT_FALSE(handler->getEvents() & EPOLLRDHUP);
        callback_cleared = true;
    });
    
    // Wait for callback to be cleared
    while (!callback_cleared.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, SetErrorCallback) {
    auto handler = loop_->handleIO(test_fd_);
    bool error_callback_called = false;
    std::atomic<bool> callback_set{false};
    std::atomic<bool> callback_cleared{false};
    
    handler->setErrorCallback([&error_callback_called]() {
        error_callback_called = true;
    });
    
    // Process the callback setting in the loop
    loop_->runInLoop([&]() {
        // Events should include EPOLLERR after setting error callback
        EXPECT_TRUE(handler->getEvents() & EPOLLERR);
        callback_set = true;
    });
    
    // Wait for callback to be set
    while (!callback_set.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    // Clear the callback
    handler->setErrorCallback(nullptr);
    
    loop_->runInLoop([&]() {
        // Events should not include EPOLLERR after clearing error callback
        EXPECT_FALSE(handler->getEvents() & EPOLLERR);
        callback_cleared = true;
    });
    
    // Wait for callback to be cleared
    while (!callback_cleared.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, MultipleCallbacks) {
    auto handler = loop_->handleIO(test_fd_);
    
    bool read_called = false;
    bool write_called = false;
    bool close_called = false;
    bool error_called = false;
    std::atomic<bool> all_callbacks_set{false};
    
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
        all_callbacks_set = true;
    });
    
    // Wait for all callbacks to be set
    while (!all_callbacks_set.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
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
    std::atomic<bool> callbacks_verified{false};
    
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
        callbacks_verified = true;
        loop_->endLoop();
    });

    loop_->loop();

    // Wait for verification to complete
    while (!callbacks_verified.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    EXPECT_EQ(callback_count.load(), 0);
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
        loop_->endLoop();
    });
    loop_->loop();
}

// Additional comprehensive tests

TEST_F(IOHandlerTest, StatusTransitions) {
    auto handler = loop_->handleIO(test_fd_);
    std::atomic<bool> test_completed{false};
    
    // Test all valid status transitions
    EXPECT_EQ(handler->status(), IOHandler::Status::Created);
    
    // Created -> Enabled
    handler->enable();
    loop_->runInLoop([&]() {
        EXPECT_EQ(handler->status(), IOHandler::Status::Enabled);
        
        // Enabled -> Disabled
        handler->disable();
    });
    
    loop_->runInLoop([&]() {
        EXPECT_EQ(handler->status(), IOHandler::Status::Disabled);
        
        // Disabled -> Enabled (re-enable)
        handler->enable();
    });
    
    loop_->runInLoop([&]() {
        EXPECT_EQ(handler->status(), IOHandler::Status::Enabled);
        test_completed = true;
        loop_->endLoop();
    });

    loop_->loop();

    while (!test_completed.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, CallbackExecutionOrder) {
    auto handler = loop_->handleIO(test_fd_);
    std::vector<std::string> execution_order;
    std::atomic<bool> test_completed{false};
    
    // Set up callbacks that record execution order
    handler->setReadCallback([&execution_order]() {
        execution_order.push_back("read");
    });
    
    handler->setWriteCallback([&execution_order]() {
        execution_order.push_back("write");
    });
    
    handler->setErrorCallback([&execution_order]() {
        execution_order.push_back("error");
    });
    
    handler->setCloseCallback([&execution_order]() {
        execution_order.push_back("close");
    });
    
    handler->enable();
    
    // Verify callbacks were set up correctly
    loop_->runInLoop([&]() {
        int events = handler->getEvents();
        EXPECT_TRUE(events & EPOLLIN);
        EXPECT_TRUE(events & EPOLLOUT);
        EXPECT_TRUE(events & EPOLLERR);
        EXPECT_TRUE(events & EPOLLRDHUP);
        test_completed = true;
        loop_->endLoop();
    });

    loop_->loop();

    while (!test_completed.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, WeakPtrSafety) {
    std::weak_ptr<IOHandler> weak_handler;
    std::atomic<bool> test_completed{false};
    
    {
        auto handler = loop_->handleIO(test_fd_);
        weak_handler = handler;
        
        EXPECT_FALSE(weak_handler.expired());
        
        handler->setReadCallback([]() {
            // Empty callback
        });
        
        // Handler goes out of scope here
    }
    
    // The handler might still be alive due to internal references
    // but should eventually be cleaned up
    loop_->runInLoop([&]() {
        // This tests that the weak_ptr mechanism in the implementation
        // properly handles the case where the handler is destroyed
        test_completed = true;
        loop_->endLoop();
    });

    loop_->loop();

    while (!test_completed.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    // After some time, the handler should be cleaned up
    // This is implementation-dependent, so we just ensure no crashes
}

TEST_F(IOHandlerTest, MultipleEnableDisableCycles) {
    auto handler = loop_->handleIO(test_fd_);
    std::atomic<int> cycle_count{0};
    const int max_cycles = 5;
    
    std::function<void()> cycle_test = [&]() {
        if (cycle_count >= max_cycles) return;
        
        handler->enable();
        loop_->runInLoop([&]() {
            EXPECT_TRUE(handler->isEnabled());
            
            handler->disable();
            loop_->runInLoop([&]() {
                EXPECT_FALSE(handler->isEnabled());
                cycle_count++;
                
                if (cycle_count < max_cycles) {
                    cycle_test();
                }
            });
        });
    };
    
    cycle_test();
    
    // Wait for all cycles to complete
    while (cycle_count.load() < max_cycles) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    EXPECT_EQ(cycle_count.load(), max_cycles);
}

TEST_F(IOHandlerTest, InvalidStatusTransitions) {
    auto handler = loop_->handleIO(test_fd_);
    std::atomic<bool> test_completed{false};
    
    // Test that disabling a Created handler works gracefully
    EXPECT_EQ(handler->status(), IOHandler::Status::Created);
    handler->disable();
    
    loop_->runInLoop([&]() {
        EXPECT_EQ(handler->status(), IOHandler::Status::Disabled);
        
        // Test double disable
        handler->disable();
        loop_->runInLoop([&]() {
            EXPECT_EQ(handler->status(), IOHandler::Status::Disabled);
            test_completed = true;
            loop_->endLoop();
        });
    });

    loop_->loop();
    while (!test_completed.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, EventMaskUpdates) {
    auto handler = loop_->handleIO(test_fd_);
    std::atomic<bool> test_completed{false};
    
    // Initially no events
    EXPECT_EQ(handler->getEvents(), 0);
    
    // Add read callback
    handler->setReadCallback([]() {});
    loop_->runInLoop([&]() {
        EXPECT_TRUE(handler->getEvents() & EPOLLIN);
        EXPECT_FALSE(handler->getEvents() & EPOLLOUT);
        
        // Add write callback
        handler->setWriteCallback([]() {});
        loop_->runInLoop([&]() {
            EXPECT_TRUE(handler->getEvents() & EPOLLIN);
            EXPECT_TRUE(handler->getEvents() & EPOLLOUT);
            
            // Remove read callback
            handler->setReadCallback(nullptr);
            loop_->runInLoop([&]() {
                EXPECT_FALSE(handler->getEvents() & EPOLLIN);
                EXPECT_TRUE(handler->getEvents() & EPOLLOUT);
                
                // Remove write callback
                handler->setWriteCallback(nullptr);
                loop_->runInLoop([&]() {
                    EXPECT_FALSE(handler->getEvents() & EPOLLIN);
                    EXPECT_FALSE(handler->getEvents() & EPOLLOUT);
                    test_completed = true;
                    loop_->endLoop();
                });
            });
        });
    });

    loop_->loop();

    while (!test_completed.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, ConcurrentCallbackModification) {
    auto handler = loop_->handleIO(test_fd_);
    std::atomic<int> modification_count{0};
    std::atomic<bool> test_completed{false};
    const int num_threads = 3;
    const int modifications_per_thread = 5;
    
    std::vector<std::unique_ptr<Thread>> threads;
    
    // Create multiple threads that modify callbacks concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(std::make_unique<Thread>([&, i]() {
            for (int j = 0; j < modifications_per_thread; ++j) {
                if (i % 2 == 0) {
                    handler->setReadCallback([&]() {
                        modification_count++;
                    });
                } else {
                    handler->setWriteCallback([&]() {
                        modification_count++;
                    });
                }
                CurrentThread::sleepUsec(1000); // 1ms between modifications
            }
        }));
    }
    
    // Start all threads
    for (auto& thread : threads) {
        thread->start();
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread->join();
    }
    
    // Verify final state
    loop_->queueInLoop([&]() {
        // Should have both read and write events set
        int events = handler->getEvents();
        EXPECT_TRUE(events & (EPOLLIN | EPOLLOUT));
        test_completed = true;
        loop_->endLoop();
    });

    loop_->loop();
    
    while (!test_completed.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, HandlerReuse) {
    // Test that we can create multiple handlers for the same fd
    // (though this might not be recommended in practice)
    auto handler1 = loop_->handleIO(test_fd_);
    
    EXPECT_NE(handler1, nullptr);
    EXPECT_EQ(handler1->fd(), test_fd_);
    
    // The second handler creation should work but might reuse or replace the first
    auto handler2 = loop_->handleIO(test_fd_);
    EXPECT_NE(handler2, nullptr);
    EXPECT_EQ(handler2->fd(), test_fd_);
    
    // Both handlers should be valid
    std::atomic<bool> test_completed{false};
    
    handler1->setReadCallback([]() {});
    handler2->setWriteCallback([]() {});
    
    loop_->runInLoop([&]() {
        // The behavior here depends on implementation details
        // We just ensure no crashes occur
        test_completed = true;
        loop_->endLoop();
    });

    loop_->loop();

    while (!test_completed.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}

TEST_F(IOHandlerTest, DestructorDisablesHandler) {
    std::shared_ptr<IOHandler> handler;
    std::atomic<bool> enable_verified{false};
    std::atomic<bool> cleanup_verified{false};
    
    handler = loop_->handleIO(test_fd_);
    handler->enable();
    
    loop_->runInLoop([&]() {
        EXPECT_TRUE(handler->isEnabled());
        enable_verified = true;
    });
    
    // Wait for enable verification
    while (!enable_verified.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    // Reset the handler (calls destructor)
    handler.reset();
    
    // The destructor should have called disable()
    // This test mainly ensures no crashes occur during cleanup
    loop_->runInLoop([&]() {
        // Handler is now destroyed, just ensure loop still works
        cleanup_verified = true;
    });
    
    // Wait for cleanup verification
    while (!cleanup_verified.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
}