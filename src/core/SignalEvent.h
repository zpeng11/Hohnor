/**
 * Events of signals
 */
#pragma once
#include "Event.h"

namespace Hohnor
{
    class SignalEvent : public Event
    {
    private:
        int signal_;
        SignalCallback callback_;

    public:
        SignalEvent(EventLoop *loop, int signal, SignalCallback callback);
        int signal() { return signal_; }
        void setCallback(SignalCallback callback) { callback_ = std::move(callback); }
        void run()override;
    };
} // namespace Hohnor
