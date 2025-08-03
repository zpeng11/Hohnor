#include "hohnor/core/IOHandler.h"
#include "hohnor/core/EventLoop.h"
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

void IOHandler::update(bool addNew) //addNew is True only when IOHandle call enable(), which works to add new context to epoll
{
    std::bind(&EventLoop::updateIOHandler, loop_, getTie(), addNew);
}

void IOHandler::run()
{
    LOG_DEBUG << "Handling event for " << eventsToString(fd(), revents_);
    if (!this->enabled())
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

void IOHandler::disable()
{
    HCHECK_NE(this->status(), Status::Created) << "You can not disable a handler when it is not enabled";
    if (this->enabled())
    {
        this->status_.store(Status::Disabled);
        update(false);
    }
    else
    {
        LOG_DEBUG << "IOHandler has been disabled do not need again";
    }
}

void IOHandler::enable()
{
    HCHECK_NE(this->status(), Status::Disabled) << "Trying to enable handler that has been disabled";
    if (this->status() == Status::Created)
    {
        this->status_.store(Status::Enabled);
        update(true);
    }
    else
    {
        LOG_WARN << "IOHandler has been enabled do not need again";
    }
}

void IOHandler::setReadCallback(ReadCallback cb)
{
    if (cb == nullptr)
    {
        events_ &= ~EPOLLIN;
    }
    else
    {
        events_ |= EPOLLIN;
        readCallback_ = std::move(cb);
    }
    if (this->enabled())
        update(false);
}

void IOHandler::setWriteCallback(WriteCallback cb)
{
    if (cb == nullptr)
    {
        events_ &= ~EPOLLOUT;
    }
    else
    {
        events_ |= EPOLLOUT;
        writeCallback_ = std::move(cb);
    }
    if (this->enabled())
        update(false);
}

void IOHandler::setCloseCallback(CloseCallback cb)
{
    if (cb == nullptr)
    {
        events_ &= ~EPOLLRDHUP;
    }
    else
    {
        events_ |= EPOLLRDHUP;
        closeCallback_ = std::move(cb);
    }
    if (this->enabled())
        update(false);
}
void IOHandler::setErrorCallback(ErrorCallback cb)
{
    if (cb == nullptr)
    {
        events_ &= ~EPOLLERR;
    }
    else
    {
        events_ |= EPOLLERR;
        errorCallback_ = std::move(cb);
    }
    if (this->enabled())
        update(false);
}