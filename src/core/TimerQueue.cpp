#include "TimerQueue.h"
#include "Timer.h"
#include <sys/timerfd.h>

namespace Hohnor
{
    int createTimerfd()
    {
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                       TFD_NONBLOCK | TFD_CLOEXEC);
        if (timerfd < 0)
        {
            LOG_SYSFATAL << "Failed in timerfd_create";
        }
        return timerfd;
    }

    struct timespec howMuchTimeFromNow(Timestamp when)
    {
        int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
        if (microseconds < 100)
        {
            microseconds = 100;
        }
        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(
            microseconds / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(
            (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
        return ts;
    }

    bool timerCmpLessThan(Hohnor::Timer *&lhs, Hohnor::Timer *&rhs)
    {
        if (lhs->expiration().microSecondsSinceEpoch() < rhs->expiration().microSecondsSinceEpoch())
            return true;
        else if (lhs->expiration().microSecondsSinceEpoch() > rhs->expiration().microSecondsSinceEpoch())
            return false;
        else
            return lhs->sequence() < rhs->sequence();
    }

    void resetTimerfd(int timerfd, Timestamp expiration)
    {
        // wake up loop by timerfd_settime()
        struct itimerspec newValue;
        memZero(&newValue, sizeof newValue);
        newValue.it_value = howMuchTimeFromNow(expiration);
        int ret = ::timerfd_settime(timerfd, 0, &newValue, NULL);
        if (ret)
        {
            LOG_SYSERR << "timerfd_settime()";
        }
    }
} // namespace Hohnor

using namespace Hohnor;

TimerQueue::TimerQueue(EventLoop *loop) : timerFd_(createTimerfd()), loop_(loop), timerFdIOHandle_(loop, timerFd_.fd()),
                                          heap_(timerCmpLessThan)
{
    timerFdIOHandle_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerFdIOHandle_.enable();
}

TimerQueue::~TimerQueue()
{
    while (heap_.size())
    {
        delete heap_.popTop();
    }
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    uint64_t howmany;
    ssize_t n = ::read(this->timerFd_.fd(), &howmany, sizeof howmany);
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof howmany)
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }

    while (heap_.size() && heap_.top()->expiration().microSecondsSinceEpoch() <= now.microSecondsSinceEpoch())
    {
        auto ptr = heap_.popTop();
        ptr->run();
        if (ptr->repeat())
        {
            ptr->restart(now);
            loop_->queueInLoop(std::bind(&TimerQueue::addTimerInLoop, this, ptr));
        }
        else
        {
            delete ptr;
        }
    }
    if (heap_.size())
    {
        resetTimerfd(timerFd_.fd(), heap_.top()->expiration());
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb,
                             Timestamp when,
                             double interval)
{
    auto ptr = new Timer(std::move(cb), when, interval);
    loop_->queueInLoop(std::bind(&TimerQueue::addTimerInLoop, this, ptr));
    return ptr->id();
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    loop_->assertInLoopThread();
    CHECK_NOTNULL(timer);
    heap_.insert(timer);
    if(heap_.top() == timer)
    {
        resetTimerfd(timerFd_.fd(), timer->expiration());
    }
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->queueInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    CHECK_NOTNULL(timerId.timer_);
    timerId.timer_->disable();
}