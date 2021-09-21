/**
 * The class that takes timers and manage them using Binary heap, its contain should only be acessed in the Loop thread
 */

#pragma once
#include "NonCopyable.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "BinaryHeap.h"
#include "IOHandler.h"
#include <set>
#include <vector>

namespace Hohnor
{
    class EventLoop;
    class Timer;
    class TimerId;
    class TimerQueue
    {
    public:
        explicit TimerQueue(EventLoop *loop);
        ~TimerQueue();

        ///
        /// Schedules the callback to be run at given time,
        /// repeats if @c interval > 0.0.
        ///
        /// Must be thread safe. Usually be called from other threads.
        TimerId addTimer(TimerCallback cb,
                         Timestamp when,
                         double interval);

        //For cancellation we do not directly discard the timer object in the heap, but set the running Functor of the specific timer to nullptr
        void cancel(TimerId timerId);

    private:
        void addTimerInLoop(Timer *timer);
        void cancelInLoop(TimerId timerId);
        // called when timerfd alarms
        void handleRead();
        FdGuard timerFd_;
        IOHandler timerFdIOHandle_;
        EventLoop *loop_;
        BinaryHeap<Timer *> heap_;
    };

} // namespace Hohnor
