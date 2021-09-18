/**
 * The class to manage signal handlers
 */

#pragma once

#include "NonCopyable.h"
#include "Callbacks.h"
#include "SignalHandler.h"
#include "IOHandler.h"

namespace Hohnor
{
    class SignalHandlerSet : public NonCopyable
    {
    public:
        explicit SignalHandlerSet(EventLoop *loop);
        ~SignalHandlerSet();

        //This is thread safe
        SignalHandlerId add(char signal, SignalCallback cb);
        //Thread safe
        void remove(SignalHandlerId id);

    private:
        void addInLoop(SignalHandler *handler);
        void removeInLoop(SignalHandlerId id);
        void handleRead();
        EventLoop *loop_;
        IOHandler signalPipeHandler_;
        std::unique_ptr<std::set<SignalHandler *>[]> sets_;
    };
} // namespace Hohnor
