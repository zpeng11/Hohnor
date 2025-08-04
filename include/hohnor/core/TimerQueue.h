/**
 * The class that takes timers and manage them using Binary heap, its contain should only be acessed in the Loop thread
 */

#pragma once
#include "hohnor/io/FdUtils.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/common/BinaryHeap.h"
#include <set>
#include <vector>

namespace Hohnor
{
    class EventLoop;
    class TimerHandler;
    class IOHandler;
    class Timestamp;
    class TimerQueue : public FdGuard
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
