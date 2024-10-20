/**
 * IO events
 */

#pragma once
#include "common/NonCopyable.h"
#include "common/Callbacks.h"
#include "EventLoop.h"

namespace Hohnor
{
    class IOHandler : NonCopyable
    {
    private:
        //do not manage life cycle of this fd
        EventLoop *loop_;
        const int fd_;
        int events_;
        int revents_;
        bool enable_;
        ReadCallback readCallback_;
        WriteCallback writeCallback_;
        CloseCallback closeCallback_;
        ErrorCallback errorCallback_;
        //update EPOLL flags in the loop
        void update(bool addNew = false);
        void runGuarded();

        std::weak_ptr<void> tie_;
        bool tied_;

    public:
        IOHandler(EventLoop *loop, int fd);
        ~IOHandler();

        int fd() { return fd_; }
        //Get current event setting
        int getEvents() { return events_; }
        //Used by event loop to put epoll result back to handler
        void retEvents(int revents) { revents_ = revents; }
        //Run the events according to revents
        void run();
        //if events are enabled
        bool enabled() { return enable_; }
        //Diable all events on this handler from the eventloop
        void disable();
        //Enable all events on this handler from the eventloop
        void enable();

        void tie(const std::shared_ptr<void>& obj);

        void setReadCallback(ReadCallback cb);
        void setWriteCallback(WriteCallback cb);
        void setCloseCallback(CloseCallback cb);
        void setErrorCallback(ErrorCallback cb);
    };

} // namespace Hohnor
