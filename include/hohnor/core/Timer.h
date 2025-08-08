/**
 * Timer events, this event is not created by the user, it should be managed by a Timerqueue
 */

#pragma once
#include "hohnor/time/Timestamp.h"
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/common/BinaryHeap.h"
#include "hohnor/io/FdUtils.h"
#include <atomic>
#include <memory>
#include <set>
#include <vector>

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
        std::atomic<bool> disabled_;
        const int64_t sequence_;
        EventLoop *loop_;
        static std::atomic<uint64_t> s_numCreated_;
    protected:
        TimerHandler(EventLoop *loop, TimerCallback callback, Timestamp when, double interval);
        void run();
        //Reload expiration with interval, so that timer run next loop
        void reloadInLoop();
    public:
        //Update callback, thread safe
        void updateCallback(TimerCallback callback);
        //Disable the timer, thread safe
        void disable();
        //Get expiration time
        Timestamp expiration() const { return expiration_; }
        //Get repeat interval
        double getRepeatInterval() const { return interval_; }
        //Check if it is a repeat timer
        inline bool isRepeat() const { return interval_ > 0.0; }
        //Check sequence number as timer id
        int64_t sequence() const { return sequence_; }
        static int64_t numCreated() { return s_numCreated_; }
        ~TimerHandler() = default;
    };

    class IOHandler;
    class Timestamp;
    class TimerQueue : public NonCopyable
    {
    friend class EventLoop;
    public:
        ~TimerQueue();
    protected:
        explicit TimerQueue(EventLoop *loop);
    private:
        ///
        /// Schedules the callback to be run at given time,
        /// repeats if @c interval > 0.0.
        ///
        /// Must be thread safe. Usually be called from other threads.
        std::shared_ptr<TimerHandler> addTimer(TimerCallback cb,
                         Timestamp when,
                         double interval);
        void addTimerInLoop(std::shared_ptr<TimerHandler> timerHandler);
        // called when timerfd alarms
        void handleRead();
        std::shared_ptr<IOHandler> timerFdIOHandle_;
        EventLoop *loop_;
        BinaryHeap<std::shared_ptr<TimerHandler>> heap_;
    };

} // namespace Hohnor
