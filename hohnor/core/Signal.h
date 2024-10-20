/**
 * Events of signals
 */
#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/common/Copyable.h"
#include <signal.h>
#include <atomic>

namespace Hohnor
{
    class SignalHandle;
    class EventLoop;
    class Signal : NonCopyable
    {
    private:
        const int signal_;
        const SignalCallback callback_;
        const int64_t sequence_;
        static std::atomic<uint64_t> s_numCreated_;

    public:
        Signal(int signal, SignalCallback callback) : signal_(signal), callback_(std::move(callback)), sequence_(s_numCreated_++) {}
        int signal() { return signal_; }
        void run(EventLoop *);
        int64_t sequence() const { return sequence_; }
        static int64_t numCreated() { return s_numCreated_; }
        ~Signal() = default;
    };

    class SignalHandle : public Copyable
    {
    public:
        SignalHandle()
            : signalEvent_(NULL),loop_(NULL)
        {
        }

        SignalHandle(Signal *signalEvent, EventLoop * loop)
            : signalEvent_(signalEvent),loop_(loop)
        {
        }
        friend class EventLoop;

        EventLoop* loop(){return loop_;}

        void cancel();
    private:
        Signal *signalEvent_;
        EventLoop * loop_;
    };
} // namespace Hohnor
