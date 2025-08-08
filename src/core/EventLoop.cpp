#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/io/Epoll.h"
#include "hohnor/io/FdUtils.h"
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
      wakeUpHandler_(), //initilize later
      timers_(),
      pendingFunctorsLock_(new Mutex()), pendingFunctors_(), signalMap_(),
      threadPool_()
{
}

std::shared_ptr<EventLoop> EventLoop::createEventLoop() {
    LOG_DEBUG << "Enter EventLoop creation factory";
    auto ptr = std::shared_ptr<EventLoop>(new EventLoop());
    //Two step creation, because IOHandle needs shared_ptr of the EventLoop which is only available after the EventLoop is fully constructed
    ptr->timers_ = new TimerQueue(ptr.get());
    //create eventfd for wake up
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
        LOG_SYSFATAL << "Fail to create eventfd for wake up";
    ptr->wakeUpHandler_ = ptr->handleIO(evtfd);
    ptr->wakeUpHandler_->setReadCallback(std::bind(&EventLoop::handleWakeUp, ptr.get()));
    ptr->wakeUpHandler_->enable();
    LOG_DEBUG << "EventLoop created " << ptr.get() << " in thread " << ptr->threadId_;
    return ptr;
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "Destroying EventLoop " << this << " in thread " << threadId_;
    if (state_ != End)
        endLoop();
    Loop::t_loopInThisThread = nullptr;
    delete timers_;
    delete poller_;
    delete pendingFunctorsLock_;
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_;
}

void EventLoop::loop()
{
    if(state_ == End)
    {
        LOG_ERROR << "EventLoop " << this << " is ended, Please create a new one to run again";
        return;
    }
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

    //Start resource clean up
    LOG_DEBUG << "EventLoop " << this << " in thread " << threadId_ << " is ended";
    LOG_DEBUG << "Reset all IOHandler and TimerHandler stored in loop object to disabled state";
    timers_->timerFdIOHandle_->status_ = IOHandler::Status::Disabled;
    timers_->timerFdIOHandle_.reset();
    wakeUpHandler_->status_ = IOHandler::Status::Disabled;
    wakeUpHandler_.reset();
    for (auto &pair : signalMap_)
        pair.second->disable();
    if (interactiveIOHandler_)
    {
        interactiveIOHandler_->status_ = IOHandler::Status::Disabled;
        interactiveIOHandler_->loop_.reset();
        //Do not reset interactiveIOHandler_ here, because closing STDIN_FILENO maynot be expected
        // interactiveIOHandler_.reset();
    }
    {
        MutexGuard guard(*pendingFunctorsLock_);
        pendingFunctors_.clear();
    }
    Loop::t_loopInThisThread = nullptr;
}

void EventLoop::runInLoop(Functor cb)
{
    if(state_ == End)
    {
        LOG_WARN << "EventLoop " << this << " is ended, can only run if in the same thread";
        if (isLoopThread())
            cb();
    }
    else{
        if (isLoopThread())
            cb();
        else
            queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    if(state_ == End)
    {
        LOG_ERROR << "EventLoop " << this << " is ended, can not queue in loop";
        return;
    }
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
    if(state_ == End){
        LOG_ERROR << "EventLoop " << this << " is ended, can not run in pool";
        return;
    }
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
    if(state_ == End)
    {
        LOG_ERROR << "EventLoop " << this << " is ended, can not wake up";
        return;
    }
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
    if(state_ != Ready && state_ != End)
    {
        LOG_DEBUG << "EventLoop " << this << " is ending, but not in Ready or End state, will wake up to end";
        wakeUp();
    }
    if(state_ == Ready){
        LOG_WARN << "Ending Eventloop In Ready state, need to run a quick loop.";
        loop(); //Will quickly enter & exit the loop because we have quit_
    }
    LOG_DEBUG << "EventLoop " << this << " in thread " << threadId_ << " is ended by call";
}

std::shared_ptr<IOHandler> EventLoop::handleIO(int fd){
    if(state_ == End)
    {
        LOG_ERROR << "EventLoop " << this << " is ended, can not handle new IO";
        return nullptr;
    }
    return std::shared_ptr<IOHandler>(new IOHandler(shared_from_this(), fd));
}

void EventLoop::updateIOHandler(std::shared_ptr<IOHandler> handler, bool addNew) //Load handle's epoll context to epoll, and manage context lifecycle. 
// addNew == true && enabled == true means creating epoll node, addNew == false && enabled == true means modifing events in epoll, enable == false means deleting epoll node and remove ownership
{
    assertInLoopThread();
    HCHECK(handler) << "Handler has been free";
    if (addNew)
    {
        HCHECK(handler->isEnabled())<< "Handler should be enabled when adding to epoll";
        poller_->add(handler->fd(), handler->getEvents(), (void *)handler.get());
    }
    else if (handler->isEnabled()) // If is it enable and not addNew, it means we are modifying the events
    {
        poller_->modify(handler->fd(), handler->getEvents(), (void *)handler.get());
    }
    else // If it is not enabled, we are removing the handler from epoll
    {
        poller_->remove(handler->fd());
    }
}

void EventLoop::removeFd(int fd)
{
    assertInLoopThread();
    poller_->remove(fd);
}

std::shared_ptr<TimerHandler> EventLoop::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    if(state_ == End)
    {
        LOG_ERROR << "EventLoop " << this << " is ended, can not add timer";
        return nullptr;
    }
    return timers_->addTimer(std::move(cb), when, interval);
}

// void EventLoop::removeTimer(TimerHandle id)
// {
//     timers_->cancel(id.timer_);
// }

void EventLoop::handleSignal(int signal, SignalAction action, SignalCallback cb)
{
    if(state_ == End)
    {
        LOG_ERROR << "EventLoop " << this << " is ended, can not handle signal";
        return;
    }
    runInLoop([this, signal, action, cb](){
        auto it = signalMap_.find(signal);
        if(it == signalMap_.end()){
            auto handle = std::shared_ptr<SignalHandler>(new SignalHandler(shared_from_this(), signal, action, cb));
            signalMap_[signal] = handle;
        }
        else{
            auto handle = it->second;
            handle->update(action, cb);
        }
    });
}

std::shared_ptr<IOHandler> EventLoop::interactiveIOHandler_ = nullptr;

void EventLoop::handleKeyboard(KeyboardCallback cb)
{
    if(state_ == End)
    {
        LOG_WARN << "EventLoop " << this << " is ended, keyboard interactive input is already disabled";
        return;
    }
    runInLoop([this, cb](){
        if (cb == nullptr){
            LOG_DEBUG << "Setup to disable interactive keyboard input";
            if (interactiveIOHandler_)
            {
                LOG_DEBUG << "Handler existing";
                // Disable the interactive IO handler by removing its read callback instead of disabling it.
                interactiveIOHandler_->setReadCallback(nullptr);
                interactiveIOHandler_->disable();
                FdUtils::resetInputInteractive();
            }
            return;
        }

        auto fuc = [cb](){
            char key;
            int ret = read(STDIN_FILENO, &key, 1);
            HCHECK_EQ(ret, 1) << "Failed to read from stdin, ret = " << ret;
            cb(key);
        };
        if (interactiveIOHandler_)
        {
            LOG_WARN << "Interactive IO handler already exists, updated to new one";
            interactiveIOHandler_->setReadCallback(std::move(fuc));
            interactiveIOHandler_->enable();
            FdUtils::setInputInteractive();
        }
        else{
            HCHECK(!interactiveIOHandler_) << "Interactive IO handler does not exist, creating a new one";
            FdUtils::setInputInteractive();
            //Create a new IOHandler for stdin
            //This will take over the ownership of stdin file descriptor
            //and set it to non-blocking and close-on-exec
            interactiveIOHandler_ = handleIO(STDIN_FILENO);
            interactiveIOHandler_->setReadCallback(std::move(fuc));
            interactiveIOHandler_->enable();
            LOG_DEBUG << "Interactive IO handler set for keyboard input";
        }});
}

bool EventLoop::isLoopThread() { return CurrentThread::tid() == threadId_; }