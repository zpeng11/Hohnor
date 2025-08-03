/**
 * Events of signals
 */
#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/common/Copyable.h"
#include "IOHandler.h"
#include "hohnor/io/SignalUtils.h"
#include <signal.h>
#include <atomic>

namespace Hohnor
{
    class EventLoop;
    class SignalHandler : NonCopyable
    {
        friend class EventLoop;
    private:
        SignalUtils::SigAction action_;
        int signal_;
        std::shared_ptr<IOHandler> ioHandler_;
        EventLoop * loop_;
        void createIOHandler(int fd, std::function<void()> cb);
    protected:
        SignalHandler(EventLoop* loop, int signal, SignalUtils::SigAction action, std::function<void()> cb = nullptr);
    public:
        SignalHandler() = delete;
        ~SignalHandler();
        SignalUtils::SigAction action() { return action_; }
        int signal() { return signal_; }
        void update(SignalUtils::SigAction action, std::function<void()> cb = nullptr);
    };
} // namespace Hohnor
