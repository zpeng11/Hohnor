#include "hohnor/thread/CountDownLatch.h"
#include "hohnor/thread/Thread.h"
#include "hohnor/thread/CurrentThread.h"
#include <gtest/gtest.h>
#include <vector>
#include <memory>

using namespace Hohnor;

void worker_thread_func(CountDownLatch* latch)
{
    latch->countDown();
}

TEST(CountDownLatchTest, SingleThread)
{
    CountDownLatch latch(1);
    EXPECT_EQ(latch.getCount(), 1);
    latch.countDown();
    EXPECT_EQ(latch.getCount(), 0);
    latch.wait(); // Should not block
}

TEST(CountDownLatchTest, MultipleThreads)
{
    const int num_threads = 5;
    CountDownLatch latch(num_threads);
    std::vector<std::unique_ptr<Thread>> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(new Thread([&latch]() {
            CurrentThread::sleepUsec(50 * 1000); // 50ms
            latch.countDown();
        }));
        threads.back()->start();
    }

    latch.wait(); // This should block until all threads have counted down
    EXPECT_EQ(latch.getCount(), 0);

    for (auto& t : threads)
    {
        t->join();
    }
}

TEST(CountDownLatchTest, WaitIsBlocking)
{
    CountDownLatch latch(1);
    bool worker_finished = false;

    Thread worker([&]() {
        latch.wait(); // This should block
        worker_finished = true;
    });

    worker.start();
    CurrentThread::sleepUsec(100 * 1000); // 100ms
    EXPECT_FALSE(worker_finished); // Worker should still be waiting

    latch.countDown(); // Release the latch
    CurrentThread::sleepUsec(50 * 1000); // 50ms to allow worker to finish
    EXPECT_TRUE(worker_finished); // Now worker should have finished

    worker.join();
}