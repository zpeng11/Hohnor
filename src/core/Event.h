/**
 * Event class
 */

#pragma once
#include "FdUtils.h"
#include "EventLoop.h"
#include "Callbacks.h"

namespace Hohnor
{
    class Event : NonCopyable //Noncopyable
    {
    private:
        EventLoop *loop_;

    public:
        Event(EventLoop *loop) { loop_ = loop; }
        EventLoop *ownerLoop() { return loop_; }
        virtual void run() = 0;
    };

} // namespace Hohnor
