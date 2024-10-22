/**
 * IO events
 */

#pragma once
#include "hohnor/common/Copyable.h"
#include "hohnor/common/Callbacks.h"

namespace Hohnor
{
    class EventLoop;
    class IOHandler : Copyable
    {
        /*
        * A Handler used to carry callback function to a fd when event happens.
        * It should be created by the user and passed to eventloop by value. 
        */
        friend class EventLoop;
    private:
        //do not manage life cycle of this fd
        const int fd_;
        //When setting up callbacks, this valuable will be automatically setup
        int events_;
        int revents_;
        ReadCallback readCallback_;
        WriteCallback writeCallback_;
        CloseCallback closeCallback_;
        ErrorCallback errorCallback_;

        //Run the events according to revents, only happens in eventloop
        void run();

        //Used by event loop to put epoll result back to handler
        void retEvents(int revents) { revents_ = revents; }

    public:
        IOHandler(int fd);
        ~IOHandler();

        int fd() { return fd_; }
        //Get current event setting
        int getEvents() { return events_; }

        void setReadCallback(ReadCallback cb);
        void setWriteCallback(WriteCallback cb);
        void setCloseCallback(CloseCallback cb);
        void setErrorCallback(ErrorCallback cb);
    };

} // namespace Hohnor
