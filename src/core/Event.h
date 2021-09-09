/**
 * Event class
 */

#pragma once
#include "FdUtils.h"
#include "EventLoop.h"

namespace Hohnor
{
    class Event : public FdGuard //Noncopyable
    {
    private:
        EventLoop *loop_;

    public:
        Event(/* args */);

        ~Event();
    };

} // namespace Hohnor
