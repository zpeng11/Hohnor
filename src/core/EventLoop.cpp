#include "EventLoop.h"
#include "Timestamp.h"
#include "IOHandler.h"
#include "TimerQueue.h"
#include "Timer.h"
#include "SignalHandlerSet.h"
#include <sys/eventfd.h>

namespace Hohnor
{
    namespace Loop
    {
        //To ensure that only one loop occupies a thread
        thread_local EventLoop *t_loopInThisThread;
    } // namespace Loop

} // namespace Hohnor

using namespace Hohnor;

EventLoop *EventLoop::loopOfCurrentThread()
{
    return Loop::t_loopInThisThread;
}

EventLoop::EventLoop()
    : poller_(), quit_(false),
      threadId_(CurrentThread::tid()),
      iteration_(0), state_(Ready),
      pollReturnTime_(Timestamp::now()),
      IOHandlers_(),
      wakeUpHandler_(), wakeUpFd_(), //initilize later
      timers_(new TimerQueue(this)), signalHandlers_(new SignalHandlerSet(this)),
      pendingFunctorsLock_(), pendingFunctors_()
{
    //verify there is single loop in a thread
    if (Loop::t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << Loop::t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else
    {
        Loop::t_loopInThisThread = this;
    }

    //create eventfd for wake up
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
        LOG_SYSFATAL << "Fail to create eventfd for wake up";
    wakeUpFd_.reset(new FdGuard(evtfd));
    wakeUpHandler_.reset(new IOHandler(this, evtfd));
    wakeUpHandler_->setReadCallback(std::bind(&EventLoop::handleWakeUp, this));
    wakeUpHandler_->enable();

    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
}

EventLoop::~EventLoop()
{
    wakeUpHandler_.reset();
    wakeUpFd_.reset();
    timers_.reset();
    signalHandlers_.reset();
    Loop::t_loopInThisThread = NULL;
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_;
}

void EventLoop::loop()
{
    assertInLoopThread();
    while (!quit_)
    {
        ++iteration_;
        state_ = Polling;
        //epoll Wait for any IO events
        auto res = poller_.wait();
        pollReturnTime_ = Timestamp::now();

        state_ = IOHandling;
        while (res.hasNext())
        {
            auto event = res.next();
            IOHandler *handler = (IOHandler *)event.data.ptr;
            auto iter = IOHandlers_.find(handler);
            CHECK_NE(iter, IOHandlers_.end()) << " handler ptr returned by epoll should be exist in the set";
            handler->retEvents(event.events);
            handler->run();
        }

        state_ = PendingHandling;
        std::vector<Functor> funcs;
        {
            MutexGuard guard(pendingFunctorsLock_);
            funcs.swap(pendingFunctors_);
        }
        for (Functor func : funcs)
        {
            CHECK_NE(func, nullptr) << " pending functors should not be nullptr";
            func();
        }
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if (isLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        MutexGuard guard(pendingFunctorsLock_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if (!isLoopThread() || state_ == PendingHandling || state_ == Ready)
    {
        wakeUp();
    }
}

void EventLoop::wakeUp()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeUpFd_->fd(), &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleWakeUp()
{
    LOG_DEBUG << "Waked up";
    uint64_t one = 1;
    ssize_t n = ::read(wakeUpFd_->fd(), &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::assertInLoopThread()
{
    if (!isLoopThread())
    {
        LOG_FATAL << "Assertion to be in loop thread failed";
    }
}

void EventLoop::endLoop()
{
    assertInLoopThread();
    quit_ = true;
    LOG_DEBUG << "EventLoop " << this << " in thread " << threadId_ << " is ended by call";
}

void EventLoop::addIOHandler(IOHandler *handler)
{
    assertInLoopThread();
    CHECK_EQ(IOHandlers_.find(handler), IOHandlers_.end()) << " the handler is already in set";
    IOHandlers_.insert(handler);
    // int event = handler->enabled() ? handler->getEvents() : 0;
    // poller_.add(handler->fd(),event, (void *) handler);
}

void EventLoop::updateIOHandler(IOHandler *handler, bool addNew)
{
    assertInLoopThread();
    CHECK_NE(IOHandlers_.find(handler), IOHandlers_.end()) << " can not find the handler in set";
    if (addNew)
    {
        CHECK(handler->enabled());
        poller_.add(handler->fd(), handler->getEvents(), (void *)handler);
    }
    else if (handler->enabled())
    {
        poller_.modify(handler->fd(), handler->getEvents(), (void *)handler);
    }
    else //diabled()
    {
        poller_.remove(handler->fd());
    }
}

void EventLoop::removeIOHandler(IOHandler *handler)
{
    assertInLoopThread();
    auto it = IOHandlers_.find(handler);
    CHECK_NE(it, IOHandlers_.end()) << " can not find the handler in set";
    IOHandlers_.erase(it);
    // poller_.remove(handler->fd());
}

bool EventLoop::hasIOHandler(IOHandler *handler)
{
    auto it = IOHandlers_.find(handler);
    return it != IOHandlers_.end();
}
void EventLoop::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    timers_->addTimer(std::move(cb), when, interval);
}

void EventLoop::removeTimer(TimerHandle id)
{
    timers_->cancel(id.ptr());
}

SignalHandlerId EventLoop::addSignalHandler(char signal, SignalCallback cb)
{
    return signalHandlers_->add(signal, std::move(cb));
}

void EventLoop::removeSignalHandler(SignalHandlerId id)
{
    signalHandlers_->remove(id);
}