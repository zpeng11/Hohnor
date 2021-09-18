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
    timerFdIOHandle_.disable();
    while(heap_.size())
    {
        delete heap_.popTop();
    }
}

void TimerQueue::handleRead()
{

}
