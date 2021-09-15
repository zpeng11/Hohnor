/**
 * IO events
 */

#pragma once
#include "Event.h"

namespace Hohnor
{
    class IOEvent :public Event
    {
    private:
        int fd_;
        int events_;
        int revents_;
        ReadCallback readCallback_;
        WriteCallback writeCallback_;
        CloseCallback closeCallback_;
        ErrorCallback errorCallback_;
    public:
        IOEvent(/* args */);
        ~IOEvent();
    };
    
} // namespace Hohnor
