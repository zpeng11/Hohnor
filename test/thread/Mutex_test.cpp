#include "hohnor/thread/Mutex.h"
#include "hohnor/thread/Thread.h"
#include <gtest/gtest.h>
#include <vector>
#include <iostream>

using namespace Hohnor;

Mutex g_mutex;
std::vector<int> g_vec;
const int kCount = 10 * 1000 * 1000;

void threadFunc()
{
    for (int i = 0; i < kCount; ++i)
    {
        MutexGuard lock(g_mutex);
        g_vec.push_back(i);
    }
}

TEST(MutexTest, BasicLocking)
{
    Mutex mutex;
    ASSERT_FALSE(mutex.isLockedByThisThread());
    mutex.lock();
    ASSERT_TRUE(mutex.isLockedByThisThread());
    mutex.unlock();
    ASSERT_FALSE(mutex.isLockedByThisThread());
}

TEST(MutexGuardTest, AutoLockUnlock)
{
    Mutex mutex;
    ASSERT_FALSE(mutex.isLockedByThisThread());
    {
        MutexGuard lock(mutex);
        ASSERT_TRUE(mutex.isLockedByThisThread());
    }
    ASSERT_FALSE(mutex.isLockedByThisThread());
}

TEST(MutexTest, ConcurrentAccess)
{
    g_vec.clear();
    const int kNumThreads = 10;
    std::vector<std::unique_ptr<Thread>> threads;
    for (int i = 0; i < kNumThreads; ++i)
    {
        threads.emplace_back(new Thread(&threadFunc));
        threads.back()->start();
    }

    for (int i = 0; i < kNumThreads; ++i)
    {
        threads[i]->join();
    }

    ASSERT_EQ(g_vec.size(), kNumThreads * kCount);
}