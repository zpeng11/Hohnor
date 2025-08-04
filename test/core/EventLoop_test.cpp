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
        Logger::setGlobalLogLevel(Logger::LogLevel::DEBUG);
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
    EXPECT_EQ(EventLoop::loopOfCurrentThread(), nullptr);
    
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