/**
 * The class to manage signal handlers
 */

#pragma once

#include "common/NonCopyable.h"
#include "common/Callbacks.h"
#include "Signal.h"
#include "IOHandler.h"

namespace Hohnor
{
    class SignalHandlerSet : public NonCopyable
    {
    public:
        explicit SignalHandlerSet(EventLoop *loop);
        ~SignalHandlerSet();

        //This is thread safe
        void add(char signal, SignalCallback cb);
        //Thread safe
        void remove(Signal* signal);

    private:
        void addInLoop(Signal* signal);
        void removeInLoop(Signal* signal);
        void handleRead();
        EventLoop *loop_;
        IOHandler signalPipeHandler_;
        std::unique_ptr<std::set<Signal *>[]> sets_;
    };
} // namespace Hohnor
