#include "EventLoop.h"
#include "Timestamp.h"
#include "IOHandler.h"
#include "TimerQueue.h"
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
    if(evtfd <0)
        LOG_SYSFATAL<<"Fail to create eventfd for wake up";
    wakeUpFd_.reset(new FdGuard(evtfd));
    wakeUpHandler_.reset(new IOHandler(this, evtfd));
    wakeUpHandler_->setReadCallback(std::bind(EventLoop::handleWakeUp,this));
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
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
            CHECK_NE(iter, IOHandlers_.end());
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
            func();
        }
    }
}