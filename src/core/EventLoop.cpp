#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/io/Epoll.h"
#include "hohnor/thread/Mutex.h"
#include "hohnor/thread/Exception.h"
#include "hohnor/thread/ThreadPool.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/core/Timer.h"
#include "hohnor/core/Timer.h"
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
    : poller_(new Epoll()), quit_(false),
      threadId_(CurrentThread::tid()),
      iteration_(0), state_(Ready),
      pollReturnTime_(Timestamp::now()),
      IOHandlers_(),
      wakeUpHandler_(), //initilize later
      timers_(new TimerQueue(this)),
      pendingFunctorsLock_(new Mutex()), pendingFunctors_(), signalMap_(),
      threadPool_()
{
    //create eventfd for wake up
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
        LOG_SYSFATAL << "Fail to create eventfd for wake up";
    wakeUpHandler_ = this->handleIO(evtfd);
    wakeUpHandler_->setReadCallback(std::bind(&EventLoop::handleWakeUp, this));
    wakeUpHandler_->enable();

    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
}

EventLoop::~EventLoop()
{
    endLoop();
    Loop::t_loopInThisThread = nullptr;
    delete poller_;
    delete timers_;
    delete pendingFunctorsLock_;
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_;
}

void EventLoop::loop()
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
    if(CurrentThread::tid() != threadId_)
    {
        LOG_WARN << "EventLoop::loop() - EventLoop " << this
                  << " was created in threadId_ = " << threadId_
                  << ", but current thread id = " <<  CurrentThread::tid()
                  << ", Updated thread id to current thread";
        threadId_ = CurrentThread::tid();
    }
    assertInLoopThread();
    while (!quit_)
    {
        ++iteration_;
        state_ = Polling;
        //epoll Wait for any IO events
        auto res = poller_->wait();
        pollReturnTime_ = Timestamp::now();

        state_ = IOHandling;
        while (res.hasNext())
        {
            auto event = res.next();
            IOHandler *handler = (IOHandler *)event.data.ptr;
            handler->retEvents(event.events);
            handler->run();
        }

        state_ = PendingHandling;
        std::vector<Functor> funcs;
        {
            MutexGuard guard(*pendingFunctorsLock_);
            funcs.swap(pendingFunctors_);
        }
        for (Functor func : funcs)
        {
            HCHECK_NE(func, nullptr) << " pending functors should not be nullptr";
            func();
        }
    }
    state_ = End;
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
        MutexGuard guard(*pendingFunctorsLock_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if (!isLoopThread() || state_ == PendingHandling || state_ == Ready)
    {
        wakeUp();
    }
}

void EventLoop::setThreadPools(size_t size)
{
    if (size > 0)
    {
        threadPool_.reset(new ThreadPool("EventLoop-ThreadPool"));
        threadPool_->start(size);
        LOG_DEBUG << "EventLoop " << this << " created thread pool with " << size << " threads";
    }
    else
    {
        threadPool_.reset();
        LOG_DEBUG << "EventLoop " << this << " disabled thread pool";
    }
}

void EventLoop::runInPool(Functor callback)
{
    if (threadPool_)
    {
        threadPool_->run(std::move(callback));
    }
    else
    {
        // If no thread pool is configured, run in the event loop thread
        runInLoop(std::move(callback));
    }
}

void EventLoop::wakeUp()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeUpHandler_->fd(), &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleWakeUp()
{
    LOG_DEBUG << "Waked up";
    uint64_t one = 1;
    ssize_t n = ::read(wakeUpHandler_->fd(), &one, sizeof one);
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
    quit_ = true;
    wakeUp();
    LOG_DEBUG << "EventLoop " << this << " in thread " << threadId_ << " is ended by call";
}

std::shared_ptr<IOHandler> EventLoop::handleIO(int fd){
    std::shared_ptr<IOHandler> handler(new IOHandler(this, fd));
    runInLoop(std::bind(&EventLoop::addIOHandler, this, handler));
    return handler;
}

void EventLoop::addIOHandler(std::shared_ptr<IOHandler> handler) //Use a set to reserve ownership of IOHandlers
{
    assertInLoopThread();
    HCHECK(!hasIOHandler(handler)) << " the handler is already in set";
    IOHandlers_.insert(handler);
}

bool EventLoop::hasIOHandler(std::shared_ptr<IOHandler> handler){
    return IOHandlers_.find(handler) != IOHandlers_.end();
}

void EventLoop::updateIOHandler(std::shared_ptr<IOHandler> handler, bool addNew) //Load handle's epoll context to epoll, and manage context lifecycle. 
// addNew == true && enabled == true means creating epoll node, addNew == false && enabled == true means modifing events in epoll, enable == false means deleting epoll node and remove ownership
{
    assertInLoopThread();
    HCHECK(handler) << "Handler has been free";
    HCHECK(hasIOHandler(handler)) << " can not find the handler in set";
    if (addNew)
    {
        HCHECK(handler->isEnabled())<< "Handler not enabled";
        poller_->add(handler->fd(), handler->getEvents(), (void *)handler.get());
    }
    else if (handler->isEnabled())
    {
        poller_->modify(handler->fd(), handler->getEvents(), (void *)handler.get());
    }
    else //diabled()
    {
        HCHECK(!handler->isEnabled())<< "This should be handler disabled case";
        poller_->remove(handler->fd());
        auto it = IOHandlers_.find(handler);
        IOHandlers_.erase(it);
    }
}

std::shared_ptr<TimerHandler> EventLoop::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    return timers_->addTimer(std::move(cb), when, interval);
}

// void EventLoop::removeTimer(TimerHandle id)
// {
//     timers_->cancel(id.timer_);
// }

std::shared_ptr<SignalHandler> EventLoop::handleSignal(int signal, SignalAction action, SignalCallback cb)
{
    auto it = signalMap_.find(signal);
    if(it == signalMap_.end()){
        auto handle = std::shared_ptr<SignalHandler>(new SignalHandler(this, signal, action, cb));
        signalMap_[signal] = handle;
        return handle;
    }
    else{
        auto handle = it->second;
        handle->update(action, cb);
        return handle;
    }
}

bool EventLoop::isLoopThread() { return CurrentThread::tid() == threadId_; }