#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include <gtest/gtest.h>
#include <string>
#include <unistd.h> // for sleep
#include <memory>

using namespace Hohnor;

void simple_thread_func()
{
    // A simple function for the thread to execute
}

TEST(ThreadTest, ConstructorAndCreation)
{
    int initial_threads = Thread::numCreated();
    Thread t1(simple_thread_func, "test_thread_1");
    EXPECT_EQ(t1.name(), "test_thread_1");
    EXPECT_FALSE(t1.started());
    EXPECT_EQ(Thread::numCreated(), initial_threads + 1);

    Thread t2(simple_thread_func);
    // The default name is "HohnorThread" + number
    EXPECT_EQ(t2.name(), "HohnorThread" + std::to_string(initial_threads + 2));
    EXPECT_FALSE(t2.started());
    EXPECT_EQ(Thread::numCreated(), initial_threads + 2);
}

TEST(ThreadTest, StartAndJoin)
{
    bool thread_ran = false;
    Thread t1([&]() {
        thread_ran = true;
        CurrentThread::sleepUsec(50 ); // 50ms
    });

    t1.start();
    EXPECT_TRUE(t1.started());
    EXPECT_GT(t1.tid(), 0);

    int result = t1.join();
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(thread_ran);
}

TEST(ThreadTest, DetachOnDestruction)
{
    bool thread_started = false;
    pid_t thread_tid = 0;
    {
        Thread t1([&]() {
            thread_started = true;
            thread_tid = CurrentThread::tid();
            // Keep the thread alive long enough to outlive the t1 object
            CurrentThread::sleepUsec(150 * 1000); // 150ms
        });
        t1.start();
        EXPECT_TRUE(t1.started());
    } // t1 goes out of scope and its destructor should detach the thread

    // Give the detached thread some time to finish its work
    CurrentThread::sleepUsec(200 * 1000); // 200ms
    EXPECT_TRUE(thread_started);
    EXPECT_GT(thread_tid, 0);
}

TEST(ThreadTest, ThreadNameIsSetInChildThread)
{
    std::string name_in_thread;
    Thread t1([&]() {
        name_in_thread = CurrentThread::name();
    }, "NameSetTest");

    t1.start();
    t1.join();
    EXPECT_EQ(name_in_thread, "NameSetTest");
}

TEST(ThreadTest, DefaultThreadNameIsSet)
{
    std::string name_in_thread;
    int thread_num = Thread::numCreated() + 1;
    std::string expected_name = "HohnorThread" + std::to_string(thread_num);
    
    Thread t1([&]() {
        name_in_thread = CurrentThread::name();
    });

    t1.start();
    t1.join();
    EXPECT_EQ(name_in_thread, expected_name);
}