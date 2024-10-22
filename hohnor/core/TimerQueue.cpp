#include "TimerQueue.h"
#include "Timer.h"
#include "EventLoop.h"
#include "hohnor/log/Logging.h"
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

    bool timerCmpLessThan(std::pair<Timestamp, std::weak_ptr<Timer>>&lhs, std::pair<Timestamp, std::weak_ptr<Timer>>&rhs)
    {
        return lhs.first().microSecondsSinceEpoch() < rhs.first().microSecondsSinceEpoch();
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

TimerQueue::TimerQueue(EventLoop * loop) : timerFd_(createTimerfd()), timers_(),
                                          heap_(timerCmpLessThan), loop_(loop)
{
    //TODO prepare a read IOHandler in Eventloop and bind with timer fd.
}

TimerQueue::~TimerQueue(){
    LOG_INFO << "Still have "<<timers_.size()<<" timers no finished";
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

    while (heap_.size() && heap_.top().first().microSecondsSinceEpoch() <= now.microSecondsSinceEpoch())
    {
        auto lockedTmr = heap_.popTop().second().lock();
        if(lockedTmr){
            lockedTmr->run(now, lockedTmr);
            if (lockedTmr->repeat())
            {
                lockedTmr->restart(now);
                loop_->queueInLoop(std::bind(&TimerQueue::addTimerInLoop, this, heap_.popTop().second()));
            }
            else
            {
                delete ptr;
            }
        }
    }
    if (heap_.size())
    {
        resetTimerfd(timerFd_.fd(), heap_.top()->expiration());
    }
}

void TimerQueue::restartTimer(std::weak_ptr<Timer> tmr){
    loop_->assertInLoopThread();
    auto lockedTmr = tmr.lock();
    if(lockedTmr){
        heap_.insert({lockedTmr->sequence(), tmr});
        if(heap_.top().second() == weakTimer){
            resetTimerfd(timerFd_.fd(), timer->expiration());
        }
    }
}

std::weak_ptr<Timer> TimerQueue::addTimer(TimerCallback cb,
                             Timestamp when,
                             double interval)
{
    loop_->assertInLoopThread();
    auto timer = std::make_shared<Timer>(cb, when, interval);
    std::weak_ptr<Timer> weakTimer = timer;
    timers_.insert({timer->sequence(), timer});
    heap_.insert({when, weakTimer});
    if(heap_.top().second() == weakTimer){
        resetTimerfd(timerFd_.fd(), timer->expiration());
    }
}


void TimerQueue::cancel(std::weak_ptr<Timer> tmr)
{
    loop_->assertInLoopThread();
    auto lockedTmr = tmr.lock();
    if(lockedTmr){
        auto sequence = lockedTmr->sequence();
        lockedTmr->disable();
        timers_.erase(sequence);
    }
}
