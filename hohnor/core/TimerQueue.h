/**
 * The class that takes timers and manage them using Binary heap, its contain should only be acessed in the Loop thread
 */

#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/common/BinaryHeap.h"
#include "hohnor/core/IOHandler.h"
#include <set>
#include <vector>

namespace Hohnor
{
    class EventLoop;
    class Timer;
    class TimerHandle;
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
        void addTimer(TimerCallback cb,
                         Timestamp when,
                         double interval);

        //For cancellation we do not directly discard the timer object in the heap, but set the running Functor of the specific timer to nullptr
        void cancel(Timer *timer);

    private:
        void addTimerInLoop(Timer *timer);
        void cancelInLoop(Timer *timer);
        // called when timerfd alarms
        void handleRead();
        FdGuard timerFd_;
        IOHandler timerFdIOHandle_;
        EventLoop *loop_;
        BinaryHeap<Timer *> heap_;
    };

} // namespace Hohnor
