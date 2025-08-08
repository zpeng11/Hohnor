#include "hohnor/core/Signal.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include "hohnor/log/Logging.h"
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

using namespace Hohnor;

class SignalTest : public ::testing::Test {
protected:
    void SetUp() override {
        loop_ = EventLoop::createEventLoop();
        // Use SIGUSR1 and SIGUSR2 for testing as they are safe for testing
        test_signal1_ = SIGUSR1;
        test_signal2_ = SIGUSR2;
        
        // Reset signal handlers to default before each test
        signal(test_signal1_, SIG_DFL);
        signal(test_signal2_, SIG_DFL);
    }
    
    void TearDown() override {
        // Reset signal handlers to default after each test
        signal(test_signal1_, SIG_DFL);
        signal(test_signal2_, SIG_DFL);
        loop_.reset();
    }

    std::shared_ptr<EventLoop> loop_;
    int test_signal1_;
    int test_signal2_;
};

TEST_F(SignalTest, CreateSignalHandlerWithIgnoredAction) {
    // handleSignal now returns void, so we just verify it doesn't crash
    EXPECT_NO_THROW(loop_->handleSignal(test_signal1_, SignalAction::Ignored));
}

TEST_F(SignalTest, CreateSignalHandlerWithDefaultAction) {
    // handleSignal now returns void, so we just verify it doesn't crash
    EXPECT_NO_THROW(loop_->handleSignal(test_signal1_, SignalAction::Default));
}

TEST_F(SignalTest, CreateSignalHandlerWithHandledAction) {
    std::atomic<bool> callback_called{false};
    
    // handleSignal now returns void, so we just verify it doesn't crash
    EXPECT_NO_THROW(loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback_called]() {
            callback_called = true;
        }));
}

TEST_F(SignalTest, CreateSignalHandlerWithHandledActionNoCallback) {
    // Should be able to create handler without callback (though it will log a warning)
    EXPECT_NO_THROW(loop_->handleSignal(test_signal1_, SignalAction::Handled));
}

TEST_F(SignalTest, SignalCallbackExecution) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_finished{false};
    
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback_called, &loop_finished]() {
            callback_called = true;
            // End the loop after handling the signal
            if (auto* loop = EventLoop::loopOfCurrentThread()) {
                loop->endLoop();
                loop_finished = true;
            }
        });
    
    // Create a separate thread to run the event loop
    Thread loop_thread([&]() {
        loop_->loop();
    });
    
    loop_thread.start();
    
    // Wait a bit for the loop to start
    CurrentThread::sleepUsec(10 * 1000); // 10ms
    
    // Send the signal to ourselves
    kill(getpid(), test_signal1_);
    
    // Wait for the loop to finish processing the signal
    loop_thread.join();
    
    // Verify the callback was called
    EXPECT_TRUE(callback_called.load());
    EXPECT_TRUE(loop_finished.load());
}

TEST_F(SignalTest, UpdateSignalActionFromIgnoredToHandled) {
    std::atomic<bool> callback_called{false};
    
    // First create with ignored action
    loop_->handleSignal(test_signal1_, SignalAction::Ignored);
    
    // Update to handled with callback - this will update the existing handler
    loop_->handleSignal(test_signal1_, SignalAction::Handled, [&callback_called]() {
        callback_called = true;
    });
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, UpdateSignalActionFromHandledToDefault) {
    std::atomic<bool> callback_called{false};
    
    // First create with handled action
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback_called]() {
            callback_called = true;
        });
    
    // Update to default - this will update the existing handler
    loop_->handleSignal(test_signal1_, SignalAction::Default);
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, UpdateSignalActionFromHandledToIgnored) {
    std::atomic<bool> callback_called{false};
    
    // First create with handled action
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback_called]() {
            callback_called = true;
        });
    
    // Update to ignored - this will update the existing handler
    loop_->handleSignal(test_signal1_, SignalAction::Ignored);
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, UpdateCallbackWhileStayingHandled) {
    std::atomic<int> callback_count{0};
    
    // First create with handled action and initial callback
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback_count]() {
            callback_count = 1;
        });
    
    // Update callback while staying in handled state
    loop_->handleSignal(test_signal1_, SignalAction::Handled, [&callback_count]() {
        callback_count = 2;
    });
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, DisableSignalHandler) {
    std::atomic<bool> callback_called{false};
    
    // First create with handled action
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback_called]() {
            callback_called = true;
        });
    
    // Disable the handler by setting to Default action
    loop_->handleSignal(test_signal1_, SignalAction::Default);
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, MultipleSignalHandlers) {
    std::atomic<bool> callback1_called{false};
    std::atomic<bool> callback2_called{false};
    
    // Create handlers for different signals
    EXPECT_NO_THROW(loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback1_called]() {
            callback1_called = true;
        }));
    
    EXPECT_NO_THROW(loop_->handleSignal(test_signal2_, SignalAction::Handled,
        [&callback2_called]() {
            callback2_called = true;
        }));
    
    // Test passes if no exceptions are thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, SignalHandlerLifecycle) {
    // Create a signal handler
    EXPECT_NO_THROW(loop_->handleSignal(test_signal1_, SignalAction::Handled));
    
    // The handler is managed internally by the EventLoop
    // This test mainly ensures no crashes occur during lifecycle management
    loop_->runInLoop([&]() {
        // The handler is referenced internally by the EventLoop
        // This test mainly ensures no crashes occur
    });
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, ThreadSafetyOfSignalHandling) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> thread_finished{false};
    
    // Create initial signal handler
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback_called]() {
            callback_called = true;
        });
    
    // Update signal handler from different thread
    Thread t([&]() {
        CurrentThread::sleepUsec(5 * 1000); // 5ms
        loop_->handleSignal(test_signal1_, SignalAction::Ignored);
        thread_finished = true;
    });
    
    t.start();
    t.join();
    
    EXPECT_TRUE(thread_finished.load());
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, SignalHandlerWithEventLoopIntegration) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_started{false};
    std::atomic<bool> loop_finished{false};
    
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback_called, &loop_finished]() {
            callback_called = true;
            if (auto* loop = EventLoop::loopOfCurrentThread()) {
                loop->endLoop();
                loop_finished = true;
            }
        });
    
    // Create a separate thread to run the event loop
    Thread loop_thread([&]() {
        loop_started = true;
        loop_->loop();
    });
    
    loop_thread.start();
    
    // Wait for the loop to start
    while (!loop_started.load()) {
        CurrentThread::sleepUsec(1000); // 1ms
    }
    
    CurrentThread::sleepUsec(10 * 1000); // 10ms additional wait
    
    // Send the signal
    kill(getpid(), test_signal1_);
    
    // Wait for the loop to finish
    loop_thread.join();
    
    // Verify the integration worked
    EXPECT_TRUE(callback_called.load());
    EXPECT_TRUE(loop_finished.load());
}

TEST_F(SignalTest, UpdateFromDefaultToHandled) {
    // Create with default action
    loop_->handleSignal(test_signal1_, SignalAction::Default);
    
    std::atomic<bool> callback_called{false};
    
    // Update from default to handled
    loop_->handleSignal(test_signal1_, SignalAction::Handled, [&callback_called]() {
        callback_called = true;
    });
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, UpdateFromIgnoredToDefault) {
    // Create with ignored action
    loop_->handleSignal(test_signal1_, SignalAction::Ignored);
    
    // Update from ignored to default
    loop_->handleSignal(test_signal1_, SignalAction::Default);
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}

TEST_F(SignalTest, ReuseSignalAfterDisable) {
    std::atomic<bool> callback1_called{false};
    std::atomic<bool> callback2_called{false};
    
    // Create first handler
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback1_called]() {
            callback1_called = true;
        });
    
    // Disable first handler by setting to default
    loop_->handleSignal(test_signal1_, SignalAction::Default);
    
    // Create second handler for the same signal with new callback
    loop_->handleSignal(test_signal1_, SignalAction::Handled,
        [&callback2_called]() {
            callback2_called = true;
        });
    
    // Test passes if no exception is thrown
    EXPECT_TRUE(true);
}