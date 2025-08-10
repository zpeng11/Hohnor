/**
 * Events of signals
 */
#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"

#include <atomic>
#include <memory>

namespace Hohnor
{
    enum SignalAction
    {
        Ignored,
        Default,
        Handled //Return signal fd that handles the signal
    };
    class EventLoop;
    typedef std::shared_ptr<EventLoop> EventLoopPtr;
    class IOHandler;
    typedef std::shared_ptr<IOHandler> IOHandlerPtr;
    class SignalHandler : NonCopyable
    {
        friend class EventLoop;
    private:
        SignalAction action_;
        int signal_;
        IOHandlerPtr ioHandler_;
        EventLoopPtr loop_;
        void createIOHandler(int fd, Functor cb);
    protected:
        SignalHandler(EventLoopPtr loop, int signal, SignalAction action, SignalCallback cb = nullptr);
    public:
        SignalHandler() = delete;
        ~SignalHandler() = default;
        SignalAction action() { return action_; }
        int signal() { return signal_; }
        void update(SignalAction action, SignalCallback cb = nullptr);
        void disable();
    };
} // namespace Hohnor
