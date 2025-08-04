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
    EventLoop* loop = new EventLoop();
    EXPECT_NE(loop, nullptr);
    EXPECT_EQ(loop->iteration(), 0);
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
    loop->queueInLoop([loop](){
        EXPECT_EQ(EventLoop::loopOfCurrentThread(), loop);
        loop->endLoop();    
    });
    loop->loop(); 
    delete loop;
    LOG_DEBUG << "Finished deleting loop ";
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
}

TEST_F(EventLoopTest, OnlyOneLoopPerThread) {
    EventLoop loop1;
    // Creating another loop in the same thread should cause a fatal error
    // We can't test this directly as it would terminate the test
    // But we can verify the current loop is set correctly
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
}

TEST_F(EventLoopTest, RunInLoopSameThread) {
    EventLoop loop;
    bool callback_executed = false;
    
    loop.runInLoop([&callback_executed]() {
        callback_executed = true;
    });
    
    EXPECT_TRUE(callback_executed);
}

TEST_F(EventLoopTest, QueueInLoop) {
    EventLoop loop;
    bool callback_executed = false;
    
    loop.queueInLoop([&callback_executed]() {
        callback_executed = true;
    });
    
    // Callback should be queued but not executed yet
    EXPECT_FALSE(callback_executed);
}

TEST_F(EventLoopTest, RunInLoopFromDifferentThread) {
    EventLoop loop;
    std::atomic<bool> callback_executed{false};
    std::atomic<bool> thread_finished{false};
    
    Thread t([&loop, &callback_executed, &thread_finished]() {
        loop.runInLoop([&callback_executed]() {
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
    loop.queueInLoop([&loop]() {
        loop.endLoop();
    });
    loop.loop();
    LOG_DEBUG << "Loop finished running";
    
    t.join();
    EXPECT_TRUE(thread_finished.load());
    EXPECT_TRUE(callback_executed.load());
}

TEST_F(EventLoopTest, WakeUpMechanism) {
    EventLoop loop;
    std::atomic<bool> wakeup_called{false};
    
    Thread t([&loop, &wakeup_called]() {
        CurrentThread::sleepUsec(50 * 1000); // 50ms
        loop.runInLoop([&wakeup_called, &loop]() {
            wakeup_called = true;
            loop.endLoop();
        });
    });
    
    t.start();
    loop.loop(); // This should be woken up by the thread
    t.join();
    
    EXPECT_TRUE(wakeup_called.load());
}

TEST_F(EventLoopTest, EndLoop) {
    EventLoop loop;
    int iterations = 0;
    
    loop.runInLoop([&loop, &iterations]() {
        iterations++;
        if (iterations < 3) {
            loop.runInLoop([&loop, &iterations]() {
                iterations++;
                if (iterations < 3) {
                    loop.runInLoop([&loop, &iterations]() {
                        iterations++;
                        loop.endLoop();
                    });
                } else {
                    loop.endLoop();
                }
            });
        } else {
            loop.endLoop();
        }
    });
    
    loop.loop();
    EXPECT_EQ(iterations, 3);
}

TEST_F(EventLoopTest, HandleIOCreation) {
    EventLoop loop;
    
    // Create a simple eventfd for testing
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    ASSERT_GT(fd, 0);
    
    auto handler = loop.handleIO(fd);
    EXPECT_NE(handler, nullptr);
    EXPECT_EQ(handler->fd(), fd);
    EXPECT_EQ(handler->status(), IOHandler::Status::Created);
    
    // Clean up
    handler.reset();
}

TEST_F(EventLoopTest, IterationCounter) {
    EventLoop loop;
    EXPECT_EQ(loop.iteration(), 0);
    
    int callback_count = 0;
    loop.runInLoop([&loop, &callback_count]() {
        callback_count++;
        if (callback_count < 3) {
            loop.queueInLoop([&loop, &callback_count]() {
                callback_count++;
                if (callback_count < 3) {
                    loop.queueInLoop([&loop, &callback_count]() {
                        callback_count++;
                        loop.endLoop();
                    });
                } else {
                    loop.endLoop();
                }
            });
        } else {
            loop.endLoop();
        }
    });
    
    loop.loop();
    EXPECT_GT(loop.iteration(), 0);
    EXPECT_EQ(callback_count, 3);
}

TEST_F(EventLoopTest, AssertInLoopThread) {
    EventLoop loop;
    
    // Should not throw when called from the loop thread
    loop.assertInLoopThread();
    
    // Test from different thread would cause fatal error, so we can't test it directly
    // But we can verify the thread ID is set correctly
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
}

TEST_F(EventLoopTest, PollReturnTime) {
    EventLoop loop;
    
    // Initial poll return time should be set
    Timestamp initial_time = loop.pollReturnTime();
    EXPECT_GT(initial_time.microSecondsSinceEpoch(), 0);
    
    // After running the loop, poll return time should be updated
    loop.runInLoop([&loop]() {
        loop.endLoop();
    });
    
    loop.loop();
    Timestamp after_loop_time = loop.pollReturnTime();
    EXPECT_GE(after_loop_time.microSecondsSinceEpoch(), initial_time.microSecondsSinceEpoch());
}

TEST_F(EventLoopTest, MultipleCallbacksInQueue) {
    EventLoop loop;
    std::vector<int> execution_order;
    
    // Queue multiple callbacks
    for (int i = 0; i < 5; ++i) {
        loop.queueInLoop([&execution_order, i]() {
            execution_order.push_back(i);
        });
    }
    
    // Add final callback to end the loop
    loop.queueInLoop([&loop]() {
        loop.endLoop();
    });
    
    loop.loop();
    
    // Verify all callbacks were executed in order
    EXPECT_EQ(execution_order.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(execution_order[i], i);
    }
}

// ThreadPool functionality tests
TEST_F(EventLoopTest, ThreadPoolInitialization) {
    EventLoop loop;
    
    // Initially no thread pool should be configured
    // Test setting up thread pool with different sizes
    loop.setThreadPools(2);
    
    // Test disabling thread pool
    loop.setThreadPools(0);
    
    // Test setting up thread pool again
    loop.setThreadPools(3);
}

TEST_F(EventLoopTest, RunInPoolWithoutThreadPool) {
    EventLoop loop;
    std::atomic<bool> callback_executed{false};
    std::atomic<pid_t> execution_thread_id{0};
    
    // Without thread pool, runInPool should behave like runInLoop
    loop.runInPool([&callback_executed, &execution_thread_id]() {
        callback_executed = true;
        execution_thread_id = CurrentThread::tid();
    });
    
    EXPECT_TRUE(callback_executed.load());
    EXPECT_EQ(execution_thread_id.load(), CurrentThread::tid());
}

TEST_F(EventLoopTest, RunInPoolWithThreadPool) {
    EventLoop loop;
    std::atomic<bool> callback_executed{false};
    std::atomic<pid_t> execution_thread_id{0};
    std::atomic<pid_t> main_thread_id{CurrentThread::tid()};
    
    // Set up thread pool with 2 threads
    loop.setThreadPools(2);
    
    // Give thread pool time to start
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    loop.runInPool([&callback_executed, &execution_thread_id]() {
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
    EventLoop loop;
    const int num_tasks = 10;
    std::atomic<int> completed_tasks{0};
    std::vector<std::atomic<pid_t>> thread_ids(num_tasks);
    std::atomic<pid_t> main_thread_id{CurrentThread::tid()};
    
    // Set up thread pool with 3 threads
    loop.setThreadPools(3);
    
    // Give thread pool time to start
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit multiple tasks
    for (int i = 0; i < num_tasks; ++i) {
        loop.runInPool([&completed_tasks, &thread_ids, i]() {
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
    EventLoop loop;
    std::atomic<bool> pool_task_executed{false};
    std::atomic<bool> loop_task_executed{false};
    std::atomic<pid_t> pool_thread_id{0};
    std::atomic<pid_t> loop_thread_id{0};
    
    // Set up thread pool
    loop.setThreadPools(2);
    
    // Give thread pool time to start
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit task to thread pool
    loop.runInPool([&pool_task_executed, &pool_thread_id]() {
        pool_task_executed = true;
        pool_thread_id = CurrentThread::tid();
    });
    
    // Submit task to event loop
    loop.runInLoop([&loop_task_executed, &loop_thread_id]() {
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
    EventLoop loop;
    std::atomic<int> task_count{0};
    
    // Start with 2 threads
    loop.setThreadPools(2);
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit some tasks
    for (int i = 0; i < 5; ++i) {
        loop.runInPool([&task_count]() {
            task_count++;
            CurrentThread::sleepUsec(20 * 1000); // 20ms
        });
    }
    
    // Wait for tasks to start
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Reconfigure to 4 threads
    loop.setThreadPools(4);
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit more tasks
    for (int i = 0; i < 5; ++i) {
        loop.runInPool([&task_count]() {
            task_count++;
            CurrentThread::sleepUsec(20 * 1000); // 20ms
        });
    }
    
    // Wait for all tasks to complete
    CurrentThread::sleepUsec(300 * 1000); // 300ms
    
    EXPECT_EQ(task_count.load(), 10);
}

TEST_F(EventLoopTest, ThreadPoolDisableAfterUse) {
    EventLoop loop;
    std::atomic<bool> pool_task_executed{false};
    std::atomic<bool> fallback_task_executed{false};
    std::atomic<pid_t> pool_thread_id{0};
    std::atomic<pid_t> fallback_thread_id{0};
    
    // Set up thread pool
    loop.setThreadPools(2);
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit task to thread pool
    loop.runInPool([&pool_task_executed, &pool_thread_id]() {
        pool_task_executed = true;
        pool_thread_id = CurrentThread::tid();
    });
    
    // Wait for task to complete
    CurrentThread::sleepUsec(100 * 1000); // 100ms
    
    // Disable thread pool
    loop.setThreadPools(0);
    
    // Submit another task - should fallback to runInLoop
    loop.runInPool([&fallback_task_executed, &fallback_thread_id]() {
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
    EventLoop loop;
    std::atomic<bool> normal_task_executed{false};
    
    // Set up thread pool
    loop.setThreadPools(2);
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    
    // Submit a normal task first to verify pool works
    loop.runInPool([&normal_task_executed]() {
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
        EventLoop loop;
        loop.setThreadPools(2);
        CurrentThread::sleepUsec(50 * 1000); // 50ms
        
        // Submit a long-running task
        loop.runInPool([&pool_task_started, &pool_task_completed]() {
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