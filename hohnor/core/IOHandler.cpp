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

IOHandler::IOHandler(int fd) :fd_(fd), events_(0), revents_(0){}

IOHandler::~IOHandler(){}

void IOHandler::run()
{
    LOG_DEBUG << "Handling event for " << eventsToString(fd_, revents_);
        
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
    else{
        LOG_WARN<<"A run on IOHandler that has not received event back";
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
        readCallback_ = cb;
    }
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
        writeCallback_ = cb;
    }
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
        closeCallback_ = cb;
    }
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
        errorCallback_ = cb;
    }
}
