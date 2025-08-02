#include "IOHandler.h"
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

IOHandler::IOHandler(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), enable_(false), tied_(false)
{
    loop_->addIOHandler(this);
}

IOHandler::~IOHandler()
{
    disable();
    loop_->removeIOHandler(this);
}

void IOHandler::update(bool addNew)
{
    loop_->updateIOHandler(this, addNew);
}

void IOHandler::run()
{
    this->runGuarded();
}

void IOHandler::runGuarded()
{
    LOG_DEBUG << "Handling event for " << eventsToString(fd_, revents_);
    if (enable_ != true)
    {
        LOG_WARN << " When running, the Handler should be enabled";
        return;
    }
        
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_ == nullptr)
            LOG_WARN << "There is not handler for CLOSE on fd:" << this->fd_;
        else
            closeCallback_();
    }
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_ == nullptr)
            LOG_WARN << "There is not handler for ERROR on fd:" << this->fd_;
        else
            errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if (readCallback_ == nullptr)
            LOG_WARN << "There is not handler for READ on fd:" << this->fd_;
        else
            readCallback_();
    }
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_ == nullptr)
            LOG_WARN << "There is not handler for WRITE on fd:" << this->fd_;
        else
            writeCallback_();
    }
}

void IOHandler::disable()
{
    if (enable_)
    {
        enable_ = false;
        update(false);
    }
    else
    {
        LOG_WARN << "IOHandler has been disabled do not need again";
    }
}

void IOHandler::enable()
{
    if (!enable_)
    {
        enable_ = true;
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
    if (enable_)
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
    if (enable_)
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
    if (enable_)
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
    if (enable_)
        update(false);
}

void IOHandler::tie(const std::shared_ptr<void>& obj)
{
  tie_ = obj;
  tied_ = true;
}