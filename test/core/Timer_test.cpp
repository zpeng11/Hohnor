#include "hohnor/core/Timer.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/log/Logging.h"
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <chrono>
#include <vector>

using namespace Hohnor;

class TimerTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop_ = EventLoop::createEventLoop();
        callback_count_ = 0;
        timer_executed_ = false;
        // Logger::setGlobalLogLevel(Logger::LogLevel::DEBUG);
    }
    
    void TearDown() override {
        loop_.reset();
    }

    std::shared_ptr<EventLoop> loop_;
    std::atomic<int> callback_count_{0};
    std::atomic<bool> timer_executed_{false};
    
    // Helper function to create a timestamp in the future
    Timestamp futureTime(double seconds) {
        return addTime(Timestamp::now(), seconds);
    }
    
    // Helper function to create a timestamp in the past
    Timestamp pastTime(double seconds) {
        return addTime(Timestamp::now(), -seconds);
    }
};

// Test TimerHandler basic functionality
TEST_F(TimerTest, TimerHandlerBasicProperties) {
    Timestamp when = futureTime(1.0);
    double interval = 0.0; // Non-repeating timer
    
    auto timer = loop_->addTimer([this]() {
        timer_executed_ = true;
    }, when, interval);
    
    EXPECT_NE(timer, nullptr);
    EXPECT_EQ(timer->sequence(), 0);
    EXPECT_EQ(timer->getRepeatInterval(), 0.0);
    EXPECT_FALSE(timer->isRepeat());
    EXPECT_GT(timer->expiration().microSecondsSinceEpoch(), Timestamp::now().microSecondsSinceEpoch());
}

TEST_F(TimerTest, TimerHandlerRepeatingTimer) {
    double interval = 0.1; // 100ms interval
    Timestamp when = futureTime(0.05);
    
    auto timer = loop_->addTimer([this]() {
        callback_count_++;
    }, when, interval);
    
    EXPECT_NE(timer, nullptr);
    EXPECT_EQ(timer->getRepeatInterval(), interval);
    EXPECT_TRUE(timer->isRepeat());
}

TEST_F(TimerTest, TimerHandlerSequenceNumbering) {
    int64_t initial_count = TimerHandler::numCreated();
    
    Timestamp when = futureTime(1.0);
    auto timer1 = loop_->addTimer([]() {}, when);
    auto timer2 = loop_->addTimer([]() {}, when);
    auto timer3 = loop_->addTimer([]() {}, when);
    
    EXPECT_EQ(timer1->sequence(), initial_count);
    EXPECT_EQ(timer2->sequence(), initial_count + 1);
    EXPECT_EQ(timer3->sequence(), initial_count + 2);
    EXPECT_EQ(TimerHandler::numCreated(), initial_count + 3);
}

TEST_F(TimerTest, TimerHandlerDisable) {
    std::atomic<bool> callback_called{false};
    
    Timestamp when = futureTime(0.01);
    auto timer = loop_->addTimer([&callback_called]() {
        callback_called = true;
    }, when);
    
    // Disable the timer before it can execute
    timer->disable();
    
    // Run the loop briefly to see if the timer would have executed
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    CurrentThread::sleepUsec(50 * 1000); // 50ms
    loop_->endLoop();
    loop_thread.join();
    
    
    // The callback should not have been called since timer was disabled
    EXPECT_FALSE(callback_called.load());
}

TEST_F(TimerTest, TimerHandlerUpdateCallback) {
    std::atomic<int> callback_version{0};
    
    Timestamp when = futureTime(0.05);
    auto timer = loop_->addTimer([&callback_version]() {
        callback_version = 1;
    }, when);
    
    // Update the callback before execution
    timer->updateCallback([&callback_version]() {
        callback_version = 2;
    });
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    CurrentThread::sleepUsec(100 * 1000); // 100ms
    loop_->endLoop();
    loop_thread.join();
    
    // Should execute the updated callback
    EXPECT_EQ(callback_version.load(), 2);
}

TEST_F(TimerTest, SingleTimerExecution) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when = futureTime(0.01);
    auto timer = loop_->addTimer([&callback_called, &loop_finished]() {
        callback_called = true;
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    }, when);
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(callback_called.load());
    EXPECT_TRUE(loop_finished.load());
}

TEST_F(TimerTest, RepeatingTimerExecution) {
    std::atomic<int> callback_count{0};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when = futureTime(0.01);
    auto timer = loop_->addTimer([&callback_count, &loop_finished]() {
        callback_count++;
        if (callback_count >= 3) {
            if (auto* loop = EventLoop::loopOfCurrentThread()) {
                loop->endLoop();
                loop_finished = true;
            }
        }
    }, when, 0.01); // 10ms interval
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_GE(callback_count.load(), 3);
    EXPECT_TRUE(loop_finished.load());
}

TEST_F(TimerTest, MultipleTimersExecution) {
    std::atomic<int> timer1_count{0};
    std::atomic<int> timer2_count{0};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when1 = futureTime(0.01);
    Timestamp when2 = futureTime(0.02);
    
    auto timer1 = loop_->addTimer([&timer1_count]() {
        timer1_count++;
    }, when1, 0.01); // 10ms interval
    
    auto timer2 = loop_->addTimer([&timer2_count, &loop_finished]() {
        timer2_count++;
        if (timer2_count >= 2) {
            if (auto* loop = EventLoop::loopOfCurrentThread()) {
                loop->endLoop();
                loop_finished = true;
            }
        }
    }, when2, 0.02); // 20ms interval
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_GE(timer1_count.load(), 2);
    EXPECT_GE(timer2_count.load(), 2);
    EXPECT_TRUE(loop_finished.load());
}

TEST_F(TimerTest, TimerOrdering) {
    std::vector<int> execution_order;
    std::atomic<bool> loop_finished{false};
    
    // Schedule timers in reverse order of execution time
    Timestamp when3 = futureTime(0.03);
    auto timer3 = loop_->addTimer([&execution_order, &loop_finished]() {
        execution_order.push_back(3);
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    }, when3);
    
    Timestamp when2 = futureTime(0.02);
    auto timer2 = loop_->addTimer([&execution_order]() {
        execution_order.push_back(2);
    }, when2);
    
    Timestamp when1 = futureTime(0.01);
    auto timer1 = loop_->addTimer([&execution_order]() {
        execution_order.push_back(1);
    }, when1);
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(loop_finished.load());
    EXPECT_EQ(execution_order.size(), 3);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
}

TEST_F(TimerTest, TimerWithSameExpiration) {
    std::vector<int64_t> execution_order;
    std::atomic<bool> loop_finished{false};
    
    Timestamp same_time = futureTime(0.01);
    
    // Create timers with the same expiration time - should execute in sequence order
    auto timer1 = loop_->addTimer([&execution_order]() {
        execution_order.push_back(1);
    }, same_time);
    
    auto timer2 = loop_->addTimer([&execution_order]() {
        execution_order.push_back(2);
    }, same_time);
    
    auto timer3 = loop_->addTimer([&execution_order, &loop_finished]() {
        execution_order.push_back(3);
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    }, same_time);
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(loop_finished.load());
    EXPECT_EQ(execution_order.size(), 3);
    // Should execute in sequence order when expiration times are the same
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
}

TEST_F(TimerTest, CancelRepeatingTimer) {
    std::atomic<int> callback_count{0};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when = futureTime(0.01);
    auto timer = loop_->addTimer([&callback_count]() {
        callback_count++;
    }, when, 0.01); // 10ms interval
    
    // Schedule cancellation after some executions
    Timestamp cancel_when = futureTime(0.05);
    loop_->addTimer([&timer, &loop_finished]() {
        timer->disable();
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            // Wait a bit more to ensure no more callbacks
            loop->addTimer([]() {
                if (auto* loop = EventLoop::loopOfCurrentThread()) {
                    loop->endLoop();
                }
            }, addTime(Timestamp::now(), 0.02));
        }
    }, cancel_when);
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    // Should have executed a few times before being cancelled
    int final_count = callback_count.load();
    EXPECT_GT(final_count, 0);
    EXPECT_LT(final_count, 10); // Should not have run too many times
}

TEST_F(TimerTest, TimerInPast) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_finished{false};
    
    // Schedule a timer in the past - should execute immediately
    Timestamp past_time = pastTime(1.0);
    auto timer = loop_->addTimer([&callback_called, &loop_finished]() {
        callback_called = true;
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    }, past_time);
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(callback_called.load());
    EXPECT_TRUE(loop_finished.load());
}

TEST_F(TimerTest, ThreadSafetyOfTimerOperations) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> thread_finished{false};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when = futureTime(0.05);
    auto timer = loop_->addTimer([&callback_called]() {
        callback_called = true;
    }, when);
    
    // Update timer from different thread
    Thread t([&]() {
        CurrentThread::sleepUsec(10 * 1000); // 10ms
        timer->updateCallback([&callback_called, &loop_finished]() {
            callback_called = true;
            if (auto* loop = EventLoop::loopOfCurrentThread()) {
                loop->endLoop();
                loop_finished = true;
            }
        });
        thread_finished = true;
    });
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    t.start();
    loop_thread.start();
    
    t.join();
    loop_thread.join();
    
    EXPECT_TRUE(thread_finished.load());
    EXPECT_TRUE(callback_called.load());
    EXPECT_TRUE(loop_finished.load());
}

TEST_F(TimerTest, TimerHandlerLifecycle) {
    std::weak_ptr<TimerHandler> weak_timer;
    std::atomic<bool> loop_finished{false};
    
    {
        Timestamp when = futureTime(0.01);
        auto timer = loop_->addTimer([&loop_finished]() {
            if (auto* loop = EventLoop::loopOfCurrentThread()) {
                loop->endLoop();
                loop_finished = true;
            }
        }, when);
        weak_timer = timer;
        EXPECT_FALSE(weak_timer.expired());
    }
    
    // Timer should still be alive due to internal references
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(loop_finished.load());
    // After execution, the timer might be cleaned up
}

TEST_F(TimerTest, ZeroIntervalTimer) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when = futureTime(0.01);
    auto timer = loop_->addTimer([&callback_called, &loop_finished]() {
        callback_called = true;
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    }, when, 0.0); // Zero interval - should not repeat
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(callback_called.load());
    EXPECT_TRUE(loop_finished.load());
    EXPECT_FALSE(timer->isRepeat());
}

TEST_F(TimerTest, NegativeIntervalTimer) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when = futureTime(0.01);
    auto timer = loop_->addTimer([&callback_called, &loop_finished]() {
        callback_called = true;
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    }, when, -1.0); // Negative interval - should not repeat
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(callback_called.load());
    EXPECT_TRUE(loop_finished.load());
    EXPECT_FALSE(timer->isRepeat());
}

TEST_F(TimerTest, UpdateCallbackAfterExpiration) {
    std::atomic<int> callback_version{0};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when = futureTime(0.01);
    auto timer = loop_->addTimer([&callback_version]() {
        callback_version = 1;
    }, when);
    
    // Schedule callback update after timer should have expired
    Timestamp update_when = futureTime(0.05);
    loop_->addTimer([&timer, &callback_version, &loop_finished]() {
        timer->updateCallback([&callback_version]() {
            callback_version = 2;
        });
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    }, update_when);
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(loop_finished.load());
    // Should have executed the original callback
    EXPECT_EQ(callback_version.load(), 1);
}

TEST_F(TimerTest, DisableAfterExpiration) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_finished{false};
    
    Timestamp when = futureTime(0.01);
    auto timer = loop_->addTimer([&callback_called]() {
        callback_called = true;
    }, when);
    
    // Schedule disable after timer should have expired
    Timestamp disable_when = futureTime(0.05);
    loop_->addTimer([&timer, &loop_finished]() {
        timer->disable();
        if (auto* loop = EventLoop::loopOfCurrentThread()) {
            loop->endLoop();
            loop_finished = true;
        }
    }, disable_when);
    
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    loop_thread.join();
    
    EXPECT_TRUE(loop_finished.load());
    EXPECT_TRUE(callback_called.load());
}

// Test utility functions
TEST_F(TimerTest, TimerComparisonFunction) {
    Timestamp now = Timestamp::now();
    Timestamp future1 = addTime(now, 1.0);
    Timestamp future2 = addTime(now, 2.0);
    
    auto timer1 = loop_->addTimer([]() {}, future1);
    auto timer2 = loop_->addTimer([]() {}, future2);
    auto timer3 = loop_->addTimer([]() {}, future1); // Same time as timer1
    
    // Test the comparison function used in binary heap
    // timer1 should be less than timer2 (earlier expiration)
    EXPECT_TRUE(timer1->expiration() < timer2->expiration());
    
    // timer1 and timer3 have same expiration, so sequence should determine order
    EXPECT_EQ(timer1->expiration(), timer3->expiration());
    EXPECT_LT(timer1->sequence(), timer3->sequence());
}

TEST_F(TimerTest, TimestampUtilityFunctions) {
    Timestamp now = Timestamp::now();
    
    // Test addTime function
    Timestamp future = addTime(now, 1.5);
    EXPECT_GT(future.microSecondsSinceEpoch(), now.microSecondsSinceEpoch());
    
    double diff = timeDifference(future, now);
    EXPECT_NEAR(diff, 1.5, 0.001); // Allow small floating point error
    
    // Test with negative time
    Timestamp past = addTime(now, -1.0);
    EXPECT_LT(past.microSecondsSinceEpoch(), now.microSecondsSinceEpoch());
    
    double negative_diff = timeDifference(past, now);
    EXPECT_NEAR(negative_diff, -1.0, 0.001);
}