#include "hohnor/thread/Condition.h"
#include "hohnor/thread/Mutex.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include <gtest/gtest.h>
#include <queue>

using namespace Hohnor;

Mutex g_mutex_cdt;
Condition g_cond(g_mutex_cdt);
std::queue<int> g_queue;
bool g_notified = false;

void producer_func()
{
    CurrentThread::sleepUsec(100 * 1000); // 100ms to ensure consumer waits
    MutexGuard lock(g_mutex_cdt);
    g_queue.push(1);
    g_notified = true;
    g_cond.notify();
}

void consumer_func()
{
    MutexGuard lock(g_mutex_cdt);
    while (!g_notified)
    {
        g_cond.wait();
    }
    ASSERT_FALSE(g_queue.empty());
    EXPECT_EQ(g_queue.front(), 1);
    g_queue.pop();
}

TEST(ConditionTest, WaitAndNotify)
{
    // Reset global state for the test
    g_notified = false;
    while(!g_queue.empty()) g_queue.pop();

    Thread producer(producer_func, "Producer");
    Thread consumer(consumer_func, "Consumer");

    consumer.start();
    producer.start();

    producer.join();
    consumer.join();

    EXPECT_TRUE(g_queue.empty());
}

TEST(ConditionTest, TimedWaitTimesOut)
{
    MutexGuard lock(g_mutex_cdt);
    // Test that timedWait times out and returns true
    bool timedout = g_cond.timedWait(0.1); // 100ms
    EXPECT_TRUE(timedout);
}

TEST(ConditionTest, TimedWaitSucceeds)
{
    g_notified = false;
    Thread producer_thread([]() {
        CurrentThread::sleepUsec(50 * 1000); // 50ms
        MutexGuard lock(g_mutex_cdt);
        g_notified = true;
        g_cond.notify();
    }, "ShortProducer");

    producer_thread.start();

    bool timedout = false;
    {
        MutexGuard lock(g_mutex_cdt);
        while(!g_notified) {
            // 200ms timeout, should not time out as producer notifies after 50ms
            timedout = g_cond.timedWait(0.2);
        }
    }
    // The loop should exit because g_notified is true, and timedout should be false
    EXPECT_FALSE(timedout);
    EXPECT_TRUE(g_notified);
    
    producer_thread.join();
}