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
        loop_ = std::make_unique<EventLoop>();
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

    std::unique_ptr<EventLoop> loop_;
    int test_signal1_;
    int test_signal2_;
};

TEST_F(SignalTest, CreateSignalHandlerWithIgnoredAction) {
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Ignored);
    
    EXPECT_NE(handler, nullptr);
    EXPECT_EQ(handler->signal(), test_signal1_);
    EXPECT_EQ(handler->action(), SignalAction::Ignored);
}

TEST_F(SignalTest, CreateSignalHandlerWithDefaultAction) {
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Default);
    
    EXPECT_NE(handler, nullptr);
    EXPECT_EQ(handler->signal(), test_signal1_);
    EXPECT_EQ(handler->action(), SignalAction::Default);
}

TEST_F(SignalTest, CreateSignalHandlerWithHandledAction) {
    std::atomic<bool> callback_called{false};
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback_called]() {
            callback_called = true;
        });
    
    EXPECT_NE(handler, nullptr);
    EXPECT_EQ(handler->signal(), test_signal1_);
    EXPECT_EQ(handler->action(), SignalAction::Handled);
}

TEST_F(SignalTest, CreateSignalHandlerWithHandledActionNoCallback) {
    // Should be able to create handler without callback (though it will log a warning)
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled);
    
    EXPECT_NE(handler, nullptr);
    EXPECT_EQ(handler->signal(), test_signal1_);
    EXPECT_EQ(handler->action(), SignalAction::Handled);
}

TEST_F(SignalTest, SignalCallbackExecution) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_finished{false};
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
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
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Ignored);
    EXPECT_EQ(handler->action(), SignalAction::Ignored);
    
    // Update to handled with callback
    handler->update(SignalAction::Handled, [&callback_called]() {
        callback_called = true;
    });
    
    EXPECT_EQ(handler->action(), SignalAction::Handled);
}

TEST_F(SignalTest, UpdateSignalActionFromHandledToDefault) {
    std::atomic<bool> callback_called{false};
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback_called]() {
            callback_called = true;
        });
    
    EXPECT_EQ(handler->action(), SignalAction::Handled);
    
    // Update to default
    handler->update(SignalAction::Default);
    
    EXPECT_EQ(handler->action(), SignalAction::Default);
}

TEST_F(SignalTest, UpdateSignalActionFromHandledToIgnored) {
    std::atomic<bool> callback_called{false};
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback_called]() {
            callback_called = true;
        });
    
    EXPECT_EQ(handler->action(), SignalAction::Handled);
    
    // Update to ignored
    handler->update(SignalAction::Ignored);
    
    EXPECT_EQ(handler->action(), SignalAction::Ignored);
}

TEST_F(SignalTest, UpdateCallbackWhileStayingHandled) {
    std::atomic<int> callback_count{0};
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback_count]() {
            callback_count = 1;
        });
    
    EXPECT_EQ(handler->action(), SignalAction::Handled);
    
    // Update callback while staying in handled state
    handler->update(SignalAction::Handled, [&callback_count]() {
        callback_count = 2;
    });
    
    EXPECT_EQ(handler->action(), SignalAction::Handled);
}

TEST_F(SignalTest, DisableSignalHandler) {
    std::atomic<bool> callback_called{false};
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback_called]() {
            callback_called = true;
        });
    
    EXPECT_EQ(handler->action(), SignalAction::Handled);
    
    // Disable the handler (should set to Default)
    handler->disable();
    
    EXPECT_EQ(handler->action(), SignalAction::Default);
}

TEST_F(SignalTest, MultipleSignalHandlers) {
    std::atomic<bool> callback1_called{false};
    std::atomic<bool> callback2_called{false};
    
    auto handler1 = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback1_called]() {
            callback1_called = true;
        });
    
    auto handler2 = loop_->handleSignal(test_signal2_, SignalAction::Handled, 
        [&callback2_called]() {
            callback2_called = true;
        });
    
    EXPECT_NE(handler1, nullptr);
    EXPECT_NE(handler2, nullptr);
    EXPECT_EQ(handler1->signal(), test_signal1_);
    EXPECT_EQ(handler2->signal(), test_signal2_);
    EXPECT_EQ(handler1->action(), SignalAction::Handled);
    EXPECT_EQ(handler2->action(), SignalAction::Handled);
}

TEST_F(SignalTest, SignalHandlerLifecycle) {
    std::weak_ptr<SignalHandler> weak_handler;
    
    {
        auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled);
        weak_handler = handler;
        EXPECT_FALSE(weak_handler.expired());
        
        // Handler should be alive while in scope
        EXPECT_EQ(handler->action(), SignalAction::Handled);
    }
    
    // After going out of scope, the handler might still be alive 
    // due to internal references in the EventLoop
    // This test mainly ensures no crashes occur
    loop_->runInLoop([&]() {
        // The handler might still be referenced internally
        // This test mainly ensures no crashes occur
    });
}

TEST_F(SignalTest, ThreadSafetyOfSignalHandling) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> thread_finished{false};
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback_called]() {
            callback_called = true;
        });
    
    // Update signal handler from different thread
    Thread t([&]() {
        CurrentThread::sleepUsec(5 * 1000); // 5ms
        handler->update(SignalAction::Ignored);
        thread_finished = true;
    });
    
    t.start();
    t.join();
    
    EXPECT_TRUE(thread_finished.load());
    EXPECT_EQ(handler->action(), SignalAction::Ignored);
}

TEST_F(SignalTest, SignalHandlerWithEventLoopIntegration) {
    std::atomic<bool> callback_called{false};
    std::atomic<bool> loop_started{false};
    std::atomic<bool> loop_finished{false};
    
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
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
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Default);
    EXPECT_EQ(handler->action(), SignalAction::Default);
    
    std::atomic<bool> callback_called{false};
    
    // Update from default to handled
    handler->update(SignalAction::Handled, [&callback_called]() {
        callback_called = true;
    });
    
    EXPECT_EQ(handler->action(), SignalAction::Handled);
}

TEST_F(SignalTest, UpdateFromIgnoredToDefault) {
    auto handler = loop_->handleSignal(test_signal1_, SignalAction::Ignored);
    EXPECT_EQ(handler->action(), SignalAction::Ignored);
    
    // Update from ignored to default
    handler->update(SignalAction::Default);
    
    EXPECT_EQ(handler->action(), SignalAction::Default);
}

TEST_F(SignalTest, ReuseSignalAfterDisable) {
    std::atomic<bool> callback1_called{false};
    std::atomic<bool> callback2_called{false};
    
    // Create first handler
    auto handler1 = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback1_called]() {
            callback1_called = true;
        });
    
    EXPECT_EQ(handler1->action(), SignalAction::Handled);
    
    // Disable first handler
    handler1->disable();
    EXPECT_EQ(handler1->action(), SignalAction::Default);
    
    // Create second handler for the same signal
    auto handler2 = loop_->handleSignal(test_signal1_, SignalAction::Handled, 
        [&callback2_called]() {
            callback2_called = true;
        });
    
    EXPECT_EQ(handler2->action(), SignalAction::Handled);
    EXPECT_NE(handler1, handler2);
}