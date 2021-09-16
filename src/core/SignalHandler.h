/**
 * Events of signals
 */
#pragma once
#include "NonCopyable.h"
#include "Callbacks.h"
#include "Copyable.h"
#include <atomic>

namespace Hohnor
{
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
        ~SignalHandler() = default;
    };
    class SignalEventId : public Copyable
    {
    public:
        SignalEventId()
            : signalEvent_(NULL),
              sequence_(0)
        {
        }

        SignalEventId(SignalHandler *signalEvent, int64_t seq)
            : signalEvent_(signalEvent),
              sequence_(seq)
        {
        }

        // default copy-ctor, dtor and assignment are okay

        friend class TimerQueue;

    private:
        SignalHandler *signalEvent_;
        int64_t sequence_;
    };
} // namespace Hohnor
