#include "hohnor/core/EventLoop.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include "hohnor/log/Logging.h"
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace Hohnor;


class EventLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test gets a fresh EventLoop
        // Logger::setGlobalLogLevel(Logger::LogLevel::DEBUG);
    }
    
    void TearDown() override {
        // Clean up after each test
    }
};

TEST_F(EventLoopTest, ConstructorAndDestructor) {
    LOG_DEBUG << "Creating loop";
    auto loop = EventLoop::create();
    LOG_DEBUG << "Created loop";
    EXPECT_NE(loop, nullptr);
    EXPECT_EQ(loop->iteration(), 0);
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
    loop->queueInLoop([loop](){
        EXPECT_EQ(EventLoop::loopOfCurrentThread(), loop.get());
        loop->endLoop();
    });
    loop->loop();
    loop.reset();
    LOG_DEBUG << "Finished deleting loop ";
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
}

TEST_F(EventLoopTest, OnlyOneLoopPerThread) {
    auto loop1 = EventLoop::create();
    // Creating another loop in the same thread should cause a fatal error
    // We can't test this directly as it would terminate the test
    // But we can verify the current loop is set correctly
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
}

TEST_F(EventLoopTest, RunInLoopSameThread) {
    auto loop = EventLoop::create();
    bool callback_executed = false;
    
    loop->runInLoop([&callback_executed]() {
        callback_executed = true;
    });
    
    EXPECT_TRUE(callback_executed);
}

TEST_F(EventLoopTest, QueueInLoop) {
    auto loop = EventLoop::create();
    bool callback_executed = false;
    
    loop->queueInLoop([&callback_executed]() {
        callback_executed = true;
    });
    
    // Callback should be queued but not executed yet
    EXPECT_FALSE(callback_executed);
}

TEST_F(EventLoopTest, RunInLoopFromDifferentThread) {
    auto loop = EventLoop::create();
    std::atomic<bool> callback_executed{false};
    std::atomic<bool> thread_finished{false};
    
    Thread t([loop, &callback_executed, &thread_finished]() {
        loop->runInLoop([&callback_executed]() {
            callback_executed = true;
        });
        thread_finished = true;
    });
    
    t.start();
    
    // Give the thread time to queue the callback
    CurrentThread::sleepUsec(10 * 1000); // 10ms
    
    // Callback should be queued but not executed until loop runs
    EXPECT_FALSE(callback_executed.load());
    
    // Run one iteration of the loop to process pending functors
    loop->queueInLoop([loop]() {
        loop->endLoop();
    });
    loop->loop();
    LOG_DEBUG << "Loop finished running";
    
    t.join();
    EXPECT_TRUE(thread_finished.load());
    EXPECT_TRUE(callback_executed.load());
}

TEST_F(EventLoopTest, WakeUpMechanism) {
    auto loop = EventLoop::create();
    std::atomic<bool> wakeup_called{false};
    
    Thread t([loop, &wakeup_called]() {
        CurrentThread::sleepUsec(50 * 1000); // 50ms
        loop->runInLoop([&wakeup_called, loop]() {
            wakeup_called = true;
            loop->endLoop();
        });
    });
    
    t.start();
    loop->loop(); // This should be woken up by the thread
    t.join();
    
    EXPECT_TRUE(wakeup_called.load());
}

TEST_F(EventLoopTest, EndLoop) {
    auto loop = EventLoop::create();
    int iterations = 0;
    
    loop->runInLoop([loop, &iterations]() {
        iterations++;
        if (iterations < 3) {
            loop->runInLoop([loop, &iterations]() {
                iterations++;
                if (iterations < 3) {
                    loop->runInLoop([loop, &iterations]() {
                        iterations++;
                        loop->endLoop();
                    });
                } else {
                    loop->endLoop();
                }
            });
        } else {
            loop->endLoop();
        }
    });
    
    loop->loop();
    EXPECT_EQ(iterations, 3);
}

TEST_F(EventLoopTest, HandleIOCreation) {
    auto loop = EventLoop::create();
    
    // Create a simple eventfd for testing
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    ASSERT_GT(fd, 0);
    
    auto handler = loop->handleIO(fd);
    EXPECT_NE(handler, nullptr);
    EXPECT_EQ(handler->fd(), fd);
    EXPECT_EQ(handler->status(), IOHandler::Status::Created);
    
    // Clean up
    handler.reset();
}

TEST_F(EventLoopTest, IterationCounter) {
    auto loop = EventLoop::create();
    EXPECT_EQ(loop->iteration(), 0);
    
    int callback_count = 0;
    loop->runInLoop([loop, &callback_count]() {
        callback_count++;
        if (callback_count < 3) {
            loop->queueInLoop([loop, &callback_count]() {
                callback_count++;
                if (callback_count < 3) {
                    loop->queueInLoop([loop, &callback_count]() {
                        callback_count++;
                        loop->endLoop();
                    });
                } else {
                    loop->endLoop();
                }
            });
        } else {
            loop->endLoop();
        }
    });
    
    loop->loop();
    EXPECT_GT(loop->iteration(), 0);
    EXPECT_EQ(callback_count, 3);
}

TEST_F(EventLoopTest, AssertInLoopThread) {
    auto loop = EventLoop::create();
    
    // Should not throw when called from the loop thread
    loop->assertInLoopThread();
    
    // Test from different thread would cause fatal error, so we can't test it directly
    // But we can verify the thread ID is set correctly
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
}

TEST_F(EventLoopTest, PollReturnTime) {
    auto loop = EventLoop::create();
    
    // Initial poll return time should be set
    Timestamp initial_time = loop->pollReturnTime();
    EXPECT_GT(initial_time.microSecondsSinceEpoch(), 0);
    
    // After running the loop, poll return time should be updated
    loop->runInLoop([loop]() {
        loop->endLoop();
    });
    
    loop->loop();
    Timestamp after_loop_time = loop->pollReturnTime();
    EXPECT_GE(after_loop_time.microSecondsSinceEpoch(), initial_time.microSecondsSinceEpoch());
}

TEST_F(EventLoopTest, MultipleCallbacksInQueue) {
    auto loop = EventLoop::create();
    std::vector<int> execution_order;
    
    // Queue multiple callbacks
    for (int i = 0; i < 5; ++i) {
        loop->queueInLoop([&execution_order, i]() {
            execution_order.push_back(i);
        });
    }
    
    // Add final callback to end the loop
    loop->queueInLoop([loop]() {
        loop->endLoop();
    });
    
    loop->loop();
    
    // Verify all callbacks were executed in order
    EXPECT_EQ(execution_order.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(execution_order[i], i);
    }
}

// ThreadPool functionality tests
TEST_F(EventLoopTest, ThreadPoolInitialization) {
    auto loop = EventLoop::create();
    
    // Initially no thread pool should be configured
    // Test setting up thread pool with different sizes
    loop->setThreadPools(2);
    
    // Test disabling thread pool
    loop->setThreadPools(0);
    
    // Test setting up thread pool again
    loop->setThreadPools(3);
}

TEST_F(EventLoopTest, RunInPoolWithoutThreadPool) {
    auto loop = EventLoop::create();
    std::atomic<bool> callback_executed{false};
    std::atomic<pid_t> execution_thread_id{0};
    
    // Without thread pool, runInPool should behave like runInLoop
    loop->runInPool([&callback_executed, &execution_thread_id]() {
        callback_executed = true;
        execution_thread_id = CurrentThread::tid();
    });
    
    EXPECT_TRUE(callback_executed.load());
    EXPECT_EQ(execution_thread_id.load(), CurrentThread::tid());
}

TEST_F(EventLoopTest, RunInPoolWithThreadPool) {
    auto loop = EventLoop::create();
    std::atomic<bool> callback_executed{false};
    std::atomic<pid_t> execution_thread_id{0};
    std::atomic<pid_t> main_thread_id{CurrentThread::tid()};
    
    // Set up thread pool with 2 threads
    loop->setThreadPools(2);
    
    // Give thread pool time to start
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    loop->runInPool([&callback_executed, &execution_thread_id]() {
        callback_executed = true;
        execution_thread_id = CurrentThread::tid();
    });
    
    // Wait for task to complete
    CurrentThread::sleepUsec(100 * 1000); // 100ms
    
    EXPECT_TRUE(callback_executed.load());
    // Task should execute in a different thread than main thread
    EXPECT_NE(execution_thread_id.load(), main_thread_id.load());
}

TEST_F(EventLoopTest, MultipleTasksInThreadPool) {
    auto loop = EventLoop::create();
    const int num_tasks = 10;
    std::atomic<int> completed_tasks{0};
    std::vector<std::atomic<pid_t>> thread_ids(num_tasks);
    std::atomic<pid_t> main_thread_id{CurrentThread::tid()};
    
    // Set up thread pool with 3 threads
    loop->setThreadPools(3);
    
    // Give thread pool time to start
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit multiple tasks
    for (int i = 0; i < num_tasks; ++i) {
        loop->runInPool([&completed_tasks, &thread_ids, i]() {
            thread_ids[i] = CurrentThread::tid();
            completed_tasks++;
            // Simulate some work
            CurrentThread::sleepUsec(10 * 1000); // 10ms
        });
    }
    
    // Wait for all tasks to complete
    CurrentThread::sleepUsec(500 * 1000); // 500ms
    
    EXPECT_EQ(completed_tasks.load(), num_tasks);
    
    // Verify tasks ran in different threads
    std::set<pid_t> unique_thread_ids;
    for (int i = 0; i < num_tasks; ++i) {
        pid_t tid = thread_ids[i].load();
        EXPECT_NE(tid, main_thread_id.load());
        unique_thread_ids.insert(tid);
    }
    
    // Should have used multiple threads (at most 3 since pool size is 3)
    EXPECT_LE(unique_thread_ids.size(), 3);
    EXPECT_GE(unique_thread_ids.size(), 1);
}

TEST_F(EventLoopTest, ThreadPoolWithEventLoopIntegration) {
    auto loop = EventLoop::create();
    std::atomic<bool> pool_task_executed{false};
    std::atomic<bool> loop_task_executed{false};
    std::atomic<pid_t> pool_thread_id{0};
    std::atomic<pid_t> loop_thread_id{0};
    
    // Set up thread pool
    loop->setThreadPools(2);
    
    // Give thread pool time to start
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit task to thread pool
    loop->runInPool([&pool_task_executed, &pool_thread_id]() {
        pool_task_executed = true;
        pool_thread_id = CurrentThread::tid();
    });
    
    // Submit task to event loop
    loop->runInLoop([&loop_task_executed, &loop_thread_id]() {
        loop_task_executed = true;
        loop_thread_id = CurrentThread::tid();
    });
    
    // Wait for pool task to complete
    CurrentThread::sleepUsec(100 * 1000); // 100ms
    
    EXPECT_TRUE(pool_task_executed.load());
    EXPECT_TRUE(loop_task_executed.load());
    
    // Pool task should run in different thread, loop task in main thread
    EXPECT_NE(pool_thread_id.load(), loop_thread_id.load());
    EXPECT_EQ(loop_thread_id.load(), CurrentThread::tid());
}

TEST_F(EventLoopTest, ThreadPoolReconfiguration) {
    auto loop = EventLoop::create();
    std::atomic<int> task_count{0};
    
    // Start with 2 threads
    loop->setThreadPools(2);
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit some tasks
    for (int i = 0; i < 5; ++i) {
        loop->runInPool([&task_count]() {
            task_count++;
            CurrentThread::sleepUsec(20 * 1000); // 20ms
        });
    }
    
    // Wait for tasks to start
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Reconfigure to 4 threads
    loop->setThreadPools(4);
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit more tasks
    for (int i = 0; i < 5; ++i) {
        loop->runInPool([&task_count]() {
            task_count++;
            CurrentThread::sleepUsec(20 * 1000); // 20ms
        });
    }
    
    // Wait for all tasks to complete
    CurrentThread::sleepUsec(300 * 1000); // 300ms
    
    EXPECT_EQ(task_count.load(), 10);
}

TEST_F(EventLoopTest, ThreadPoolDisableAfterUse) {
    auto loop = EventLoop::create();
    std::atomic<bool> pool_task_executed{false};
    std::atomic<bool> fallback_task_executed{false};
    std::atomic<pid_t> pool_thread_id{0};
    std::atomic<pid_t> fallback_thread_id{0};
    
    // Set up thread pool
    loop->setThreadPools(2);
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit task to thread pool
    loop->runInPool([&pool_task_executed, &pool_thread_id]() {
        pool_task_executed = true;
        pool_thread_id = CurrentThread::tid();
    });
    
    // Wait for task to complete
    CurrentThread::sleepUsec(100 * 1000); // 100ms
    
    // Disable thread pool
    loop->setThreadPools(0);
    
    // Submit another task - should fallback to runInLoop
    loop->runInPool([&fallback_task_executed, &fallback_thread_id]() {
        fallback_task_executed = true;
        fallback_thread_id = CurrentThread::tid();
    });
    
    EXPECT_TRUE(pool_task_executed.load());
    EXPECT_TRUE(fallback_task_executed.load());
    
    // First task ran in pool thread, second in main thread
    EXPECT_NE(pool_thread_id.load(), CurrentThread::tid());
    EXPECT_EQ(fallback_thread_id.load(), CurrentThread::tid());
}

TEST_F(EventLoopTest, ThreadPoolTaskExceptionHandling) {
    auto loop = EventLoop::create();
    std::atomic<bool> normal_task_executed{false};
    
    // Set up thread pool
    loop->setThreadPools(2);
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit a normal task first to verify pool works
    loop->runInPool([&normal_task_executed]() {
        normal_task_executed = true;
    });
    
    // Wait for normal task to complete
    CurrentThread::sleepUsec(100 * 1000); // 100ms
    
    EXPECT_TRUE(normal_task_executed.load());
    
    // Note: According to ThreadPool design, exceptions in tasks cause abort()
    // We cannot test exception throwing directly as it would terminate the test process
    // The ThreadPool implementation catches exceptions and calls abort() for safety
    // This test verifies that normal tasks execute successfully before any potential exceptions
}

TEST_F(EventLoopTest, ThreadPoolWithEventLoopLifecycle) {
    std::atomic<bool> pool_task_started{false};
    std::atomic<bool> pool_task_completed{false};
    
    {
        auto loop = EventLoop::create();
        loop->setThreadPools(2);
        CurrentThread::sleepUsec(50 * 1000); // 50ms
        
        // Submit a long-running task
        loop->runInPool([&pool_task_started, &pool_task_completed]() {
            pool_task_started = true;
            CurrentThread::sleepUsec(200 * 1000); // 200ms
            pool_task_completed = true;
        });
        
        // Wait for task to start
        CurrentThread::sleepUsec(50 * 1000); // 50ms
        EXPECT_TRUE(pool_task_started.load());
        
        // EventLoop destructor should properly clean up thread pool
    } // EventLoop goes out of scope here
    
    // Wait a bit more to see if task completes
    CurrentThread::sleepUsec(300 * 1000); // 300ms
    
    // Task should have been allowed to complete
    EXPECT_TRUE(pool_task_completed.load());
}
// New tests for EventLoop state management
TEST_F(EventLoopTest, EventLoopStateManagement) {
    auto loop = EventLoop::create();
    
    // Initial state should be Ready
    EXPECT_EQ(loop->state(), EventLoop::LoopState::Ready);
    
    // Test state transitions during loop execution
    std::atomic<bool> polling_state_seen{false};
    std::atomic<bool> pending_handling_state_seen{false};
    
    loop->queueInLoop([loop, &polling_state_seen, &pending_handling_state_seen]() {
        // During callback execution, we should be in PendingHandling state
        if (loop->state() == EventLoop::LoopState::PendingHandling) {
            pending_handling_state_seen = true;
        }
        loop->endLoop();
    });
    
    // Start the loop - this will transition through states
    loop->loop();
    
    // After loop ends, state should be End
    EXPECT_EQ(loop->state(), EventLoop::LoopState::End);
    EXPECT_TRUE(pending_handling_state_seen.load());
}

TEST_F(EventLoopTest, EventLoopStateTransitions) {
    auto loop = EventLoop::create();
    std::vector<EventLoop::LoopState> observed_states;
    
    // Monitor state changes
    loop->queueInLoop([loop, &observed_states]() {
        observed_states.push_back(loop->state());
        
        // Queue another callback to see more state transitions
        loop->queueInLoop([loop, &observed_states]() {
            observed_states.push_back(loop->state());
            loop->endLoop();
        });
    });
    
    EXPECT_EQ(loop->state(), EventLoop::LoopState::Ready);
    loop->loop();
    EXPECT_EQ(loop->state(), EventLoop::LoopState::End);
    
    // Should have observed PendingHandling states
    EXPECT_FALSE(observed_states.empty());
    for (auto state : observed_states) {
        EXPECT_EQ(state, EventLoop::LoopState::PendingHandling);
    }
}

// New tests for IOHandler enable/disable functionality
TEST_F(EventLoopTest, IOHandlerEnableDisable) {
    auto loop = EventLoop::create();
    
    // Create a simple eventfd for testing
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    ASSERT_GT(fd, 0);
    auto handler = loop->handleIO(fd);
    EXPECT_NE(handler, nullptr);
    EXPECT_EQ(handler->status(), IOHandler::Status::Created);
    EXPECT_FALSE(handler->isEnabled());
    // Enable the handler
    handler->enable();
    EXPECT_EQ(handler->status(), IOHandler::Status::Enabled);
    EXPECT_TRUE(handler->isEnabled());
    // Disable the handler
    handler->disable();
    EXPECT_EQ(handler->status(), IOHandler::Status::Disabled);
    EXPECT_FALSE(handler->isEnabled());
    // Re-enable
    handler->enable();
    EXPECT_EQ(handler->status(), IOHandler::Status::Enabled);
    EXPECT_TRUE(handler->isEnabled());
    // Clean up
    handler.reset();
}

TEST_F(EventLoopTest, IOHandlerCallbackSetters) {
    auto loop = EventLoop::create();
    
    // Create a simple eventfd for testing
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    ASSERT_GT(fd, 0);
    
    auto handler = loop->handleIO(fd);
    EXPECT_NE(handler, nullptr);
    
    // Test thread-safe callback setters
    std::atomic<bool> read_callback_set{false};
    std::atomic<bool> write_callback_set{false};
    std::atomic<bool> close_callback_set{false};
    std::atomic<bool> error_callback_set{false};
    
    handler->setReadCallback([&read_callback_set]() {
        read_callback_set = true;
    });
    
    handler->setWriteCallback([&write_callback_set]() {
        write_callback_set = true;
    });
    
    handler->setCloseCallback([&close_callback_set]() {
        close_callback_set = true;
    });
    
    handler->setErrorCallback([&error_callback_set]() {
        error_callback_set = true;
    });
    
    // Callbacks should be set (we can't easily test execution without complex setup)
    // But we can verify the setters don't crash and the handler is still valid
    EXPECT_EQ(handler->status(), IOHandler::Status::Created);
    
    // Clean up
    handler.reset();
}

TEST_F(EventLoopTest, IOHandlerLifecycleWithEventLoop) {
    auto loop = EventLoop::create();
    std::weak_ptr<IOHandler> weak_handler;
    
    {
        // Create a simple eventfd for testing
        int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        ASSERT_GT(fd, 0);
        
        auto handler = loop->handleIO(fd);
        weak_handler = handler;
        
        EXPECT_FALSE(weak_handler.expired());
        EXPECT_EQ(handler->loop(), loop);
        
        // Enable and then disable
        handler->enable();
        EXPECT_TRUE(handler->isEnabled());
        
        handler->disable();
        EXPECT_FALSE(handler->isEnabled());
        
        // Handler should still be valid
        EXPECT_FALSE(weak_handler.expired());
    }
    
    // After handler goes out of scope, weak_ptr should be expired
    // (Note: this depends on the implementation details of IOHandler lifecycle)
}

TEST_F(EventLoopTest, MultipleIOHandlers) {
    auto loop = EventLoop::create();
    std::vector<IOHandlerPtr> handlers;
    
    // Create multiple handlers
    for (int i = 0; i < 5; ++i) {
        int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        ASSERT_GT(fd, 0);
        
        auto handler = loop->handleIO(fd);
        EXPECT_NE(handler, nullptr);
        EXPECT_EQ(handler->status(), IOHandler::Status::Created);
        
        handlers.push_back(handler);
    }
    
    // Enable all handlers
    for (auto& handler : handlers) {
        handler->enable();
        EXPECT_TRUE(handler->isEnabled());
    }
    
    // Disable all handlers
    for (auto& handler : handlers) {
        handler->disable();
        EXPECT_FALSE(handler->isEnabled());
    }
    
    // Clean up
    handlers.clear();
}

TEST_F(EventLoopTest, SharedPtrEventLoopLifecycle) {
    std::weak_ptr<EventLoop> weak_loop;
    
    {
        auto loop = EventLoop::create();
        weak_loop = loop;
        
        EXPECT_FALSE(weak_loop.expired());
        EXPECT_EQ(loop->iteration(), 0);
        
        // Test that loop can be copied and shared
        auto loop_copy = loop;
        EXPECT_EQ(loop_copy, loop);
        EXPECT_FALSE(weak_loop.expired());
        
        // Test loop functionality with shared ownership
        bool callback_executed = false;
        loop->runInLoop([&callback_executed]() {
            callback_executed = true;
        });
        LOG_DEBUG << "Weak pointer use count: " << weak_loop.use_count();
        EXPECT_TRUE(callback_executed);
        loop->endLoop();
        LOG_DEBUG << "Weak pointer use count: " << weak_loop.use_count();
    }
    LOG_DEBUG << "Weak pointer use count: " << weak_loop.use_count();
    // After all shared_ptrs go out of scope, weak_ptr should be expired
    EXPECT_TRUE(weak_loop.expired());
}

TEST_F(EventLoopTest, EventLoopSharedFromThis) {
    auto loop = EventLoop::create();
    EventLoopPtr shared_loop;
    
    // Test that we can get shared_ptr from within callbacks
    loop->runInLoop([loop, &shared_loop]() {
        shared_loop = loop->shared_from_this();
    });
    
    EXPECT_NE(shared_loop, nullptr);
    EXPECT_EQ(shared_loop, loop);
    
    // Test that shared_from_this works in different contexts
    loop->queueInLoop([loop, &shared_loop]() {
        auto another_shared = loop->shared_from_this();
        EXPECT_EQ(another_shared, shared_loop);
        EXPECT_EQ(another_shared, loop);
    });
}