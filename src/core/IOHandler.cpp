#include "hohnor/core/IOHandler.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/io/Epoll.h"
#include <iostream>
#include <sstream>

namespace Hohnor
{
    string eventsToString(int fd, int ev)
    {
        std::ostringstream oss;
        oss << fd << ": ";
        if (ev & EPOLLIN)
            oss << "IN ";
        if (ev & EPOLLPRI)
            oss << "PRI ";
        if (ev & EPOLLOUT)
            oss << "OUT ";
        if (ev & EPOLLHUP)
            oss << "HUP ";
        if (ev & EPOLLRDHUP)
            oss << "RDHUP ";
        if (ev & EPOLLERR)
            oss << "ERR ";
        return oss.str();
    }
} // namespace Hohnor

using namespace Hohnor;

IOHandler::IOHandler(EventLoopPtr loop, int fd) : loop_(loop), events_(0), revents_(0), status_(Status::Created),
                                                                closeCallback_(nullptr), errorCallback_(nullptr), readCallback_(nullptr), writeCallback_(nullptr)
{
    HCHECK(loop) << "EventLoop cannot be null";
    HCHECK(fd >= 0) << "File descriptor must be non-negative";
    LOG_DEBUG << "Creating IOHandler for fd " << fd;
    this->setFd(fd);
}

IOHandler::~IOHandler()
{
    cleanCallbacks();
    //We can assure only one thread can access this IOHandler, so we can safely disable it
    LOG_DEBUG << "Destroying IOHandler as well as guard for fd " << fd();
    if(LIKELY(loop_)){
        if(LIKELY(status_ == Status::Enabled)){ //This IOHandler is still enabled
            if(UNLIKELY(loop_->state() == EventLoop::LoopState::End || loop_->quit_))
            {
                //If the loop is ended,
                //we cannot & do not need to remove the fd from epoll, 
                //so we just set self status to Disabled
                LOG_DEBUG << "EventLoop is ended, IOHandler for fd " << fd() << " will not be removed from epoll";
            }
            else {
                LOG_DEBUG << "Using loop's removeFd for fd " << fd();
                int fd = this->fd();
                EventLoop *loop = loop_.get();
                loop_->runInLoop([loop, fd]() {
                    loop->removeFd(fd);
                });
                //If the loop is not ended, we can safely remove the fd from epoll
            }
        }
        status_ = Status::Disabled;
        loop_.reset();
    }
    else{
        //This is only prepared for the case interactiveIOHandler_.
        //at this time loop_->quit_==true, loop_->state_==End,
        //and handler has been disabled
        HCHECK(status_ == Status::Disabled);
        LOG_DEBUG << "Loop has been released beforehand, nothing to do in dtor for fd " << fd();
    }
}

void IOHandler::updateInLoop(IOHandlerPtr handler, Status nextStatus) //addNew is True only when IOHandle call enable(), which works to add new context to epoll
{
    loop_->assertInLoopThread();
    bool addNew = false;
    if((status_ == Status::Created && nextStatus == Status::Enabled) || (status_ == Status::Disabled && nextStatus == Status::Enabled)) //When switching from Created/Disabled to Enabled, we need to add new context to epoll
        addNew = true;
    status_ = nextStatus;
    loop_->updateIOHandler(handler, addNew);
    if(status_ == Status::Disabled){
        cleanCallbacks();
    }
}

void IOHandler::run()
{
    LOG_TRACE << "Handling event for " << eventsToString(fd(), revents_);
    if (!this->isEnabled())
    {
        LOG_WARN << " The handler is disabled during running, probably from anther thread";
        return;
    }
        
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_ == nullptr)
            LOG_WARN << "There is not handler for CLOSE on fd:" << fd();
        else
            closeCallback_();
    }
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_ == nullptr)
            LOG_WARN << "There is not handler for ERROR on fd:" << fd();
        else
            errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if (readCallback_ == nullptr)
            LOG_WARN << "There is not handler for READ on fd:" << fd();
        else
            readCallback_();
    }
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_ == nullptr)
            LOG_WARN << "There is not handler for WRITE on fd:" << fd();
        else
            writeCallback_();
    }
}

void IOHandler::update(Status nextStatus){
    auto handler = shared_from_this();
    loop_->runInLoop([handler, nextStatus](){
        if(handler->status() == Status::Disabled && nextStatus == Status::Disabled)
        {
            LOG_DEBUG << "Trying to disable a handler that has already been disabled";
            return;
        }
        if (handler->status() == Status::Created && nextStatus == Status::Disabled)
        {
            LOG_WARN << "Trying to disable a handler that has not been enabled";
            handler->status_ = Status::Disabled;
            return;
        }
        handler->updateInLoop(handler, nextStatus);
    });
}



void IOHandler::disable()
{
    LOG_DEBUG << "Disabling IOHandler for fd " << fd();
    cleanCallbacks();
    if(loop_ && loop_->quit_){
        LOG_DEBUG << "Disabling Handler after loop has quit";
        //This means we do not need to take care about fd and epoll events
        //because they will not trigger. Only need to handle status of self
        status_ = Status::Disabled;
    }
    else
    {
        LOG_DEBUG << "Disabling Handler while loop is running";
        //This means we need to take care about fd and epoll events
        //because they may still trigger. Need to remove from epoll
        //in loop using the update()
        update(Status::Disabled);
    }
}

void IOHandler::enable()
{
    //need to update the status in loop
    update(Status::Enabled);
}

void IOHandler::setReadCallback(ReadCallback cb)
{
    HCHECK(loop_) << "EventLoop cannot be null";
    LOG_DEBUG << "Setting read callback for IOHandler on fd " << fd();
    auto handler = shared_from_this();
    loop_->runInLoop([handler, cb](){
        handler->readCallback_ = std::move(cb);
        handler->setReadEvent(cb != nullptr);
    });
}

void IOHandler::setWriteCallback(WriteCallback cb)
{
    auto handler = shared_from_this();
    loop_->runInLoop([handler, cb](){
        handler->writeCallback_ = std::move(cb);
        handler->setWriteEvent(cb != nullptr);
    });
}

void IOHandler::setCloseCallback(CloseCallback cb)
{
    auto handler = shared_from_this();
    loop_->runInLoop([handler, cb](){
        handler->closeCallback_ = std::move(cb);
        handler->setCloseEvent(cb != nullptr);
    });
}
void IOHandler::setErrorCallback(ErrorCallback cb)
{
    auto handler = shared_from_this();
    loop_->runInLoop([handler, cb](){
        handler->errorCallback_ = std::move(cb);
        handler->setErrorEvent(cb != nullptr);
    });
}

void IOHandler::setReadEvent(bool on)
{
    auto handler = shared_from_this();
    loop_->runInLoop([handler, on](){
        if (on)
        {
            handler->events_ |= EPOLLIN;
        }
        else
        {
            handler->events_ &= ~EPOLLIN;
        }
        if (handler->isEnabled())
            handler->updateInLoop(handler, Status::Enabled);
    });
}

void IOHandler::setWriteEvent(bool on)
{
    auto handler = shared_from_this();
    loop_->runInLoop([handler, on](){
        if (on)
        {
            handler->events_ |= EPOLLOUT;
        }
        else
        {
            handler->events_ &= ~EPOLLOUT;
        }
        if (handler->isEnabled())
            handler->updateInLoop(handler, Status::Enabled);
    });
}

void IOHandler::setCloseEvent(bool on)
{
    auto handler = shared_from_this();
    loop_->runInLoop([handler, on](){
        if (on)
        {
            handler->events_ |= EPOLLRDHUP;
        }
        else
        {
            handler->events_ &= ~EPOLLRDHUP;
        }
        if (handler->isEnabled())
            handler->updateInLoop(handler, Status::Enabled);
    });
}

void IOHandler::setErrorEvent(bool on)
{
    auto handler = shared_from_this();
    loop_->runInLoop([handler, on](){
        if (on)
        {
            handler->events_ |= EPOLLERR;
        }
        else
        {
            handler->events_ &= ~EPOLLERR;
        }
        if (handler->isEnabled())
            handler->updateInLoop(handler, Status::Enabled);
    });
}