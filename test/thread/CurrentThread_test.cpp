#include "hohnor/thread/CurrentThread.h"
#include "hohnor/thread/Thread.h"
#include <gtest/gtest.h>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void threadFunc_()
{
    ASSERT_GT(Hohnor::CurrentThread::tid(), 0);
    ASSERT_EQ(Hohnor::CurrentThread::name(), "test thread");
}

TEST(CurrentThreadTest, Tid)
{
    ASSERT_GT(Hohnor::CurrentThread::tid(), 0);
}

TEST(CurrentThreadTest, Name)
{
    ASSERT_EQ(Hohnor::CurrentThread::name(), "main");
}

TEST(CurrentThreadTest, IsMainThread)
{
    ASSERT_TRUE(Hohnor::CurrentThread::isMainThread());
}

TEST(CurrentThreadTest, Sleep)
{
    Hohnor::CurrentThread::sleepUsec(1000);
}

TEST(CurrentThreadTest, ThreadTid)
{
    Hohnor::Thread t(threadFunc_, "test thread");
    t.start();
    t.join();
}