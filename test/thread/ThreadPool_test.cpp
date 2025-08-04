#include "hohnor/thread/ThreadPool.h"
#include "hohnor/thread/CurrentThread.h"
#include <gtest/gtest.h>
#include <string>
#include <atomic>
#include <vector>
#include <chrono>
#include <thread>

using namespace Hohnor;

TEST(ThreadPoolTest, ConstructorAndBasicProperties)
{
    ThreadPool pool("TestPool");
    EXPECT_EQ(pool.name(), "TestPool");
    EXPECT_EQ(pool.queueCapacity(), 42); // Default queue size
    EXPECT_EQ(pool.queueSize(), 0);
    EXPECT_FALSE(pool.full());
    EXPECT_EQ(pool.poolSize(), 0);
    EXPECT_EQ(pool.busyThreads(), 0);
}

TEST(ThreadPoolTest, DefaultConstructor)
{
    ThreadPool pool;
    EXPECT_EQ(pool.name(), "ThreadPool");
    EXPECT_EQ(pool.queueCapacity(), 42);
    EXPECT_EQ(pool.queueSize(), 0);
    EXPECT_FALSE(pool.full());
    EXPECT_EQ(pool.poolSize(), 0);
    EXPECT_EQ(pool.busyThreads(), 0);
}

TEST(ThreadPoolTest, SetMaxQueueSize)
{
    ThreadPool pool;
    
    // Test normal size
    pool.setMaxQueueSize(10);
    EXPECT_EQ(pool.queueCapacity(), 10);
    
    // Test size less than 2 (should default to 42)
    pool.setMaxQueueSize(1);
    EXPECT_EQ(pool.queueCapacity(), 42);
    
    // Test size of 0 (should default to 42)
    pool.setMaxQueueSize(0);
    EXPECT_EQ(pool.queueCapacity(), 42);
    
    // Test large size
    pool.setMaxQueueSize(100);
    EXPECT_EQ(pool.queueCapacity(), 100);
}

TEST(ThreadPoolTest, StartAndStop)
{
    ThreadPool pool("StartStopTest");
    
    // Start with 3 threads
    pool.start(3);
    EXPECT_EQ(pool.poolSize(), 3);
    
    // Give threads time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Stop the pool
    pool.stop();
    EXPECT_EQ(pool.poolSize(), 3); // Threads still exist but stopped
}

TEST(ThreadPoolTest, TaskExecution)
{
    ThreadPool pool("TaskTest");
    std::atomic<int> counter(0);
    
    pool.start(2);
    
    // Submit multiple tasks
    for (int i = 0; i < 10; ++i) {
        pool.run([&counter]() {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
    }
    
    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.stop();
    
    EXPECT_EQ(counter.load(), 10);
}

TEST(ThreadPoolTest, TaskExecutionWithoutThreads)
{
    ThreadPool pool("NoThreadTest");
    std::atomic<int> counter(0);
    
    // Don't start any threads (pool size = 0)
    // Tasks should execute immediately in the calling thread
    pool.run([&counter]() {
        counter++;
    });
    
    EXPECT_EQ(counter.load(), 1);
    EXPECT_EQ(pool.poolSize(), 0);
}

TEST(ThreadPoolTest, PreThreadCallback)
{
    ThreadPool pool("PreThreadTest");
    std::atomic<int> preThreadCount(0);
    
    pool.setPreThreadCallback([&preThreadCount]() {
        preThreadCount++;
    });
    
    pool.start(3);
    
    // Give threads time to start and execute pre-thread callback
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    pool.stop();
    
    EXPECT_EQ(preThreadCount.load(), 3);
}

TEST(ThreadPoolTest, PostThreadCallback)
{
    ThreadPool pool("PostThreadTest");
    std::atomic<int> postThreadCount(0);
    
    pool.setPostThreadCallback([&postThreadCount]() {
        postThreadCount++;
    });
    
    pool.start(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    pool.stop();
    
    EXPECT_EQ(postThreadCount.load(), 2);
}

TEST(ThreadPoolTest, PreTaskCallback)
{
    ThreadPool pool("PreTaskTest");
    std::atomic<int> preTaskCount(0);
    std::atomic<int> taskCount(0);
    
    pool.setPreTaskCallback([&preTaskCount]() {
        preTaskCount++;
    });
    
    pool.start(2);
    
    // Submit tasks
    for (int i = 0; i < 5; ++i) {
        pool.run([&taskCount]() {
            taskCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.stop();
    
    EXPECT_EQ(preTaskCount.load(), 5);
    EXPECT_EQ(taskCount.load(), 5);
}

TEST(ThreadPoolTest, PostTaskCallback)
{
    ThreadPool pool("PostTaskTest");
    std::atomic<int> postTaskCount(0);
    std::atomic<int> taskCount(0);
    
    pool.setPostTaskCallback([&postTaskCount]() {
        postTaskCount++;
    });
    
    pool.start(2);
    
    // Submit tasks
    for (int i = 0; i < 5; ++i) {
        pool.run([&taskCount]() {
            taskCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.stop();
    
    EXPECT_EQ(postTaskCount.load(), 5);
    EXPECT_EQ(taskCount.load(), 5);
}

TEST(ThreadPoolTest, BusyThreadsCount)
{
    ThreadPool pool("BusyTest");
    std::atomic<bool> taskStarted(false);
    std::atomic<bool> canFinish(false);
    
    pool.start(2);
    
    // Submit a long-running task
    pool.run([&taskStarted, &canFinish]() {
        taskStarted = true;
        while (!canFinish) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Wait for task to start
    while (!taskStarted) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Check busy threads count
    EXPECT_EQ(pool.busyThreads(), 1);
    
    // Allow task to finish
    canFinish = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_EQ(pool.busyThreads(), 0);
    pool.stop();
}

TEST(ThreadPoolTest, QueueSizeAndFullStatus)
{
    ThreadPool pool("QueueTest");
    pool.setMaxQueueSize(3);
    
    std::atomic<bool> canFinish(false);
    
    // Start with 1 thread to control task execution
    pool.start(1);
    
    // Submit tasks that will block
    for (int i = 0; i < 4; ++i) {
        pool.run([&canFinish]() {
            while (!canFinish) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Give time for queue to fill up
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Queue should be full (capacity 3, with 1 task running and 3 queued)
    EXPECT_TRUE(pool.full());
    EXPECT_EQ(pool.queueSize(), 3);
    
    // Allow tasks to finish
    canFinish = true;
    pool.stop();
}

TEST(ThreadPoolTest, MultipleTaskTypes)
{
    ThreadPool pool("MultiTaskTest");
    std::atomic<int> intResult(0);
    std::atomic<double> doubleResult(0.0);
    std::string stringResult;
    
    pool.start(3);
    
    // Submit different types of tasks
    pool.run([&intResult]() {
        intResult = 42;
    });
    
    pool.run([&doubleResult]() {
        doubleResult = 3.14;
    });
    
    pool.run([&stringResult]() {
        stringResult = "Hello ThreadPool";
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    pool.stop();
    
    EXPECT_EQ(intResult.load(), 42);
    EXPECT_DOUBLE_EQ(doubleResult.load(), 3.14);
    EXPECT_EQ(stringResult, "Hello ThreadPool");
}

TEST(ThreadPoolTest, DestructorStopsRunningPool)
{
    std::atomic<int> taskCount(0);
    
    {
        ThreadPool pool("DestructorTest");
        pool.start(2);
        
        // Submit tasks
        for (int i = 0; i < 5; ++i) {
            pool.run([&taskCount]() {
                taskCount++;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            });
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
    } // pool destructor should call stop()
    
    // All submitted tasks should have been processed
    EXPECT_EQ(taskCount.load(), 5);
}

TEST(ThreadPoolTest, PreThreadCallbackWithZeroThreads)
{
    ThreadPool pool("ZeroThreadTest");
    std::atomic<int> preThreadCount(0);
    
    pool.setPreThreadCallback([&preThreadCount]() {
        preThreadCount++;
    });
    
    // Start with 0 threads - preThreadCallback should still be called once
    pool.start(0);
    
    EXPECT_EQ(preThreadCount.load(), 1);
    EXPECT_EQ(pool.poolSize(), 0);
}

TEST(ThreadPoolTest, TaskExecutionOrder)
{
    ThreadPool pool("OrderTest");
    std::vector<int> results;
    std::atomic<int> counter(0);
    
    // Use single thread to ensure order
    pool.start(1);
    
    // Submit tasks in order
    for (int i = 0; i < 5; ++i) {
        pool.run([&results, &counter, i]() {
            results.push_back(i);
            counter++;
        });
    }
    
    // Wait for all tasks to complete
    while (counter.load() < 5) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    pool.stop();
    
    // Tasks should be executed in order with single thread
    EXPECT_EQ(results.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(results[i], i);
    }
}

TEST(ThreadPoolTest, StressTest)
{
    ThreadPool pool("StressTest");
    std::atomic<int> taskCount(0);
    const int numTasks = 1000;
    
    pool.setMaxQueueSize(50);
    pool.start(4);
    
    // Submit many tasks
    for (int i = 0; i < numTasks; ++i) {
        pool.run([&taskCount]() {
            taskCount++;
            // Simulate some work
            volatile int dummy = 0;
            for (int j = 0; j < 100; ++j) {
                dummy += j;
            }
        });
    }
    
    // Wait for all tasks to complete
    while (taskCount.load() < numTasks) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    pool.stop();
    EXPECT_EQ(taskCount.load(), numTasks);
}