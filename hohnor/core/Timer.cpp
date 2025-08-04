#include "Timer.h"
#include "EventLoop.h"
#include "TimerQueue.h"
#include "IOHandler.h"
#include "hohnor/log/Logging.h"
#include <sys/timerfd.h>

using namespace Hohnor;

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

bool timerCmpLessThan(std::shared_ptr<TimerHandler> &lhs, std::shared_ptr<TimerHandler> &rhs)
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

std::atomic<uint64_t> TimerHandler::s_numCreated_;

TimerHandler::TimerHandler(EventLoop *loop, TimerCallback callback, Timestamp when, double interval) : loop_(loop), callback_(callback),
                                                                        expiration_(when),
                                                                        interval_(interval),
                                                                        disabled_(false),
                                                                        sequence_(s_numCreated_++)
{
}

void TimerHandler::run()
{
    if (!disabled_)
    {
        callback_();
    }
}

void TimerHandler::disable()
{
    std::weak_ptr<TimerHandler> weak_ptr = shared_from_this();
    loop_->runInLoop([weak_ptr](){
        auto handler = weak_ptr.lock();
        if(handler){
            handler->interval_ = 0.0;
            handler->disabled_ = true;
        }
        else{
            LOG_WARN << "Calling TimerHandler disable after object is gone";
        }
    });
}

void TimerHandler::updateCallback(TimerCallback callback)
{
    std::weak_ptr<TimerHandler> weak_ptr = shared_from_this();
    loop_->runInLoop([weak_ptr, callback](){
        auto handler = weak_ptr.lock();
        if(handler){
            handler->callback_ = std::move(callback);
            if(Timestamp::now() >= handler->expiration() && handler->repeatInterval() <= 0.0)
            {
                LOG_WARN << "Updated timer callback after time expired and no more repeat";
            }
        }
        else{
            LOG_WARN << "Calling TimerHandler updateCallback after object is gone";
        }
    });
}

void TimerHandler::reloadInLoop()
{
    if (interval_ > 0.0)
    {
        expiration_ = addTime(expiration_, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}

TimerQueue::TimerQueue(EventLoop *loop) : loop_(loop), timerFdIOHandle_(nullptr),
                                          heap_(timerCmpLessThan)
{
    setFd(createTimerfd());
    timerFdIOHandle_ = loop->handleIO(fd());
    timerFdIOHandle_->setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerFdIOHandle_->enable();
}

TimerQueue::~TimerQueue()
{
    while (heap_.size())
    {
        heap_.popTop();
    }
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    uint64_t howmany;
    ssize_t n = ::read(this->fd(), &howmany, sizeof howmany);
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (UNLIKELY(n != sizeof howmany))
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }

    while (heap_.size() && heap_.top()->expiration().microSecondsSinceEpoch() <= now.microSecondsSinceEpoch())
    {
        auto timerHandler = heap_.popTop();
        timerHandler->run();
        if (timerHandler->isRepeat())
        {
            timerHandler->reloadInLoop();
            loop_->queueInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timerHandler));
        }
        else //For non-repeating timers, mark it as disabled.
        {
            timerHandler->disabled_ = true;
        }
    }
    if (heap_.size())
    {
        resetTimerfd(this->fd(), heap_.top()->expiration());
    }
}

std::shared_ptr<TimerHandler> TimerQueue::addTimer(TimerCallback cb,
                             Timestamp when,
                             double interval)
{
    std::shared_ptr<TimerHandler> timerHandler(new TimerHandler(loop_, std::move(cb), when, interval));
    loop_->queueInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timerHandler));
    return timerHandler;
}

void TimerQueue::addTimerInLoop(std::shared_ptr<TimerHandler> timerHandler)
{
    loop_->assertInLoopThread();
    HCHECK_NE(timerHandler, nullptr) << "Adding a nullptr to timerqueue";
    heap_.insert(timerHandler);
    if(heap_.top() == timerHandler)
    {
        resetTimerfd(this->fd(), timerHandler->expiration());
    }
}