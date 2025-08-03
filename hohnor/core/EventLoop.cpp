#include "hohnor/core/EventLoop.h"
#include "hohnor/time/Timestamp.h"
#include "IOHandler.h"
#include "TimerQueue.h"
#include "Timer.h"
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
      wakeUpHandler_(), //initilize later
      timers_(new TimerQueue(this)),
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
    wakeUpHandler_.reset(new IOHandler(this, evtfd));
    wakeUpHandler_->setReadCallback(std::bind(&EventLoop::handleWakeUp, this));
    wakeUpHandler_->enable();

    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
}

EventLoop::~EventLoop()
{
    wakeUpHandler_.reset();
    timers_.reset();
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
            EpollContext *context = (EpollContext *)event.data.ptr;
            auto handler = context->tie.lock();
            if (LIKELY(handler && handler->enabled())){
                handler->retEvents(event.events);
                handler->run();
            }
            else
            {
                LOG_WARN << "Possiblly unresponded to an IO event due to disable asynchronize";
            }
        }

        state_ = PendingHandling;
        std::vector<Functor> funcs;
        {
            MutexGuard guard(pendingFunctorsLock_);
            funcs.swap(pendingFunctors_);
        }
        for (Functor func : funcs)
        {
            HCHECK_NE(func, nullptr) << " pending functors should not be nullptr";
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
    assertInLoopThread();
    quit_ = true;
    LOG_DEBUG << "EventLoop " << this << " in thread " << threadId_ << " is ended by call";
}

std::shared_ptr<IOHandler> EventLoop::handleIO(int fd){
    auto handler = std::make_shared<IOHandler>(this, fd);
    handler->tie();
    runInLoop(std::bind(&EventLoop::addIOHandler, this, handler));
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

void EventLoop::updateIOHandler(std::weak_ptr<IOHandler> handler_weak, bool addNew) //Load handle's epoll context to epoll, and manage context lifecycle. 
// addNew == true && enabled == true means creating epoll node, addNew == false && enabled == true means modifing events in epoll, enable == false means deleting epoll node and remove ownership
{
    assertInLoopThread();
    auto handler = handler_weak.lock();
    HCHECK(handler) << "Handler has been free";
    HCHECK(hasIOHandler(handler)) << " can not find the handler in set";
    if (addNew)
    {
        HCHECK(handler->enabled())<< "Handler not enabled";
        HCHECK(!handler->context)<< "Handler's epoll context has been created";
        handler->context = new EpollContext(handler, handler->fd());
        poller_.add(handler->fd(), handler->getEvents(), (void *)handler->context);
    }
    else if (handler->enabled())
    {
        HCHECK(handler->context)<< "Handler's epoll context has not been created";
        poller_.modify(handler->fd(), handler->getEvents(), (void *)handler->context);
    }
    else //diabled()
    {
        HCHECK(handler->context)<< "Handler's epoll context has not been created";
        poller_.remove(handler->fd());
        delete handler->context;
        handler->context = nullptr;
        auto it = IOHandlers_.find(handler);
        IOHandlers_.erase(it);
    }
}

void EventLoop::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    timers_->addTimer(std::move(cb), when, interval);
}

void EventLoop::removeTimer(TimerHandle id)
{
    timers_->cancel(id.timer_);
}
