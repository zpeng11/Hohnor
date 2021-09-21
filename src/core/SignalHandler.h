/**
 * Events of signals
 */
#pragma once
#include "NonCopyable.h"
#include "Callbacks.h"
#include "Copyable.h"
#include <signal.h>
#include <atomic>

namespace Hohnor
{
    class SignalHandlerId;
    class SignalHandler : NonCopyable
    {
    private:
        const int signal_;
        const SignalCallback callback_;
        const int64_t sequence_;
        static std::atomic<uint64_t> s_numCreated_;

    public:
        SignalHandler(int signal, SignalCallback callback) : signal_(signal), callback_(std::move(callback)), sequence_(s_numCreated_++) {}
        int signal() { return signal_; }
        void run() { callback_(); }
        int64_t sequence() const { return sequence_; }
        static int64_t numCreated() { return s_numCreated_; }
        SignalHandlerId id();
        ~SignalHandler() = default;
    };
    class SignalHandlerId : public Copyable
    {
    public:
        SignalHandlerId()
            : signalEvent_(NULL),
              sequence_(0)
        {
        }

        SignalHandlerId(SignalHandler *signalEvent, int64_t seq)
            : signalEvent_(signalEvent),
              sequence_(seq)
        {
        }
        friend class SignalHandlerSet;

    private:
        SignalHandler *signalEvent_;
        int64_t sequence_;
    };
} // namespace Hohnor
