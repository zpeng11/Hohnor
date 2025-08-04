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

IOHandler::IOHandler(EventLoop *loop, int fd) : loop_(loop), events_(0), revents_(0), status_(Status::Created)
{
    this->setFd(fd);
}

IOHandler::~IOHandler()
{
    disable();
}

void IOHandler::updateInLoop(std::shared_ptr<IOHandler> handler, Status nextStatus) //addNew is True only when IOHandle call enable(), which works to add new context to epoll
{
    loop_->assertInLoopThread();
    bool addNew = false;
    if(status_ == Status::Created && nextStatus == Status::Enabled)
        addNew = true;
    status_ = nextStatus;
    loop_->updateIOHandler(handler, addNew);
}

void IOHandler::run()
{
    LOG_DEBUG << "Handling event for " << eventsToString(fd(), revents_);
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
    std::weak_ptr<IOHandler> weak_self = shared_from_this();
    loop_->runInLoop([weak_self, nextStatus](){
        auto handler = weak_self.lock();
        if(handler){
            handler->updateInLoop(handler, nextStatus);
        }
        else{
            LOG_WARN << "Calling IOHandler disable multiple times";
        }
    });
}



void IOHandler::disable()
{
    HCHECK_NE(this->status(), Status::Created) << "You can not disable a handler when it is not enabled";
    update(Status::Disabled);
}

void IOHandler::enable()
{
    HCHECK_NE(this->status(), Status::Disabled) << "Trying to enable handler that has been disabled";
    update(Status::Enabled);
}

void IOHandler::setReadCallback(ReadCallback cb)
{
    std::weak_ptr<IOHandler> weak_self = shared_from_this();
    loop_->runInLoop([weak_self, cb](){
        auto handler = weak_self.lock();
        if(handler){
            if (cb == nullptr)
            {
                handler->events_ &= ~EPOLLIN;
            }
            else
            {
                handler->events_ |= EPOLLIN;
                handler->readCallback_ = std::move(cb);
            }
            if (handler->isEnabled())
                handler->updateInLoop(handler, Status::Enabled);
        }
        else{
            LOG_WARN << "Calling IOHandler disable multiple times";
        }
    });
}

void IOHandler::setWriteCallback(WriteCallback cb)
{
    std::weak_ptr<IOHandler> weak_self = shared_from_this();
    loop_->runInLoop([weak_self, cb](){
        auto handler = weak_self.lock();
        if(handler){
            if (cb == nullptr)
            {
                handler->events_ &= ~EPOLLOUT;
            }
            else
            {
                handler->events_ |= EPOLLOUT;
                handler->writeCallback_ = std::move(cb);
            }
            if (handler->isEnabled())
                handler->updateInLoop(handler, Status::Enabled);
        }
        else{
            LOG_WARN << "Calling IOHandler disable multiple times";
        }
    });
}

void IOHandler::setCloseCallback(CloseCallback cb)
{
    std::weak_ptr<IOHandler> weak_self = shared_from_this();
    loop_->runInLoop([weak_self, cb](){
        auto handler = weak_self.lock();
        if(handler){
            if (cb == nullptr)
            {
                handler->events_ &= ~EPOLLRDHUP;
            }
            else
            {
                handler->events_ |= EPOLLRDHUP;
                handler->closeCallback_ = std::move(cb);
            }
            if (handler->isEnabled())
                handler->updateInLoop(handler, Status::Enabled);
        }
        else{
            LOG_WARN << "Calling IOHandler disable multiple times";
        }
    });
}
void IOHandler::setErrorCallback(ErrorCallback cb)
{
    std::weak_ptr<IOHandler> weak_self = shared_from_this();
    loop_->runInLoop([weak_self, cb](){
        auto handler = weak_self.lock();
        if(handler){
            if (cb == nullptr)
            {
                handler->events_ &= ~EPOLLERR;
            }
            else
            {
                handler->events_ |= EPOLLERR;
                handler->errorCallback_ = std::move(cb);
            }
            if (handler->isEnabled())
                handler->updateInLoop(handler, Status::Enabled);
        }
        else{
            LOG_WARN << "Calling IOHandler disable multiple times";
        }
    });
}