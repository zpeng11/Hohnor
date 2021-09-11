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
        /* data */
    public:
        IOEvent(/* args */);
        ~IOEvent();
    };
    
    IOEvent::IOEvent(/* args */)
    {
    }
    
    IOEvent::~IOEvent()
    {
    }
    
} // namespace Hohnor
