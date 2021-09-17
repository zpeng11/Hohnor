/**
 * IO events
 */

#pragma once
#include "NonCopyable.h"
#include "Callbacks.h"
#include "EventLoop.h"

namespace Hohnor
{
    class IOHandler : NonCopyable
    {
    private:
        //do not manage life cycle of this fd
        EventLoop * loop_;
        const int fd_;
        int events_;
        int revents_;
        ReadCallback readCallback_;
        WriteCallback writeCallback_;
        CloseCallback closeCallback_;
        ErrorCallback errorCallback_;
        //update EPOLL flags in the loop
        void update();

    public:
        IOHandler(EventLoop *loop, int fd);
        ~IOHandler();
        int getEvents() { return events_; }
        void retEvents(int revents) { revents_ = revents; }
        void run();
        void disable();
        void enable();
        void setReadCallback(ReadCallback cb)
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
            update();
        }
        void setWriteCallback(WriteCallback cb)
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
            update();
        }
        void setCloseCallback(CloseCallback cb)
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
            update();
        }
        void setErrorCallback(ErrorCallback cb)
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
            update();
        }
        ~IOHandler();
    };

} // namespace Hohnor
