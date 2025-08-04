/**
 * Timer events, this event is not created by the user, it should be managed by a Timerqueue
 */

#pragma once
#include "hohnor/time/Timestamp.h"
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include <atomic>
#include <memory>

namespace Hohnor
{
    class EventLoop;
    class TimerQueue;
    class TimerHandler : NonCopyable, public std::enable_shared_from_this<TimerHandler> 
    {
        friend class TimerQueue;
    private:
        TimerCallback callback_;
        Timestamp expiration_;
        //if greater than 0 means it is a repeated event
        double interval_;
        bool disabled_;
        const int64_t sequence_;
        EventLoop *loop_;
        static std::atomic<uint64_t> s_numCreated_;
    protected:
        TimerHandler(EventLoop *loop, TimerCallback callback, Timestamp when, double interval);
        void run();
        //Reload expiration with interval, so that timer run next loop
        void reloadInLoop();
    public:
        void updateCallback(TimerCallback callback);
        void disable();
        Timestamp expiration() const { return expiration_; }
        double repeatInterval() const { return interval_; }
        inline bool isRepeat() const { return interval_ > 0.0; }
        int64_t sequence() const { return sequence_; }
        static int64_t numCreated() { return s_numCreated_; }
        ~TimerHandler() = default;
    };

} // namespace Hohnor
