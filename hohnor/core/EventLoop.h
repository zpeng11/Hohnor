/**
 * The core of the Hohnor library manages events in loop,
 * which is simular to a IO event dispatcher
 */


#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/thread/BlockingQueue.h"
#include "hohnor/io/Epoll.h"
#include "hohnor/common/BinaryHeap.h"
#include "hohnor/core/Timer.h"
#include "hohnor/core/Signal.h"
#include <atomic>
#include <vector>
#include <set>
namespace Hohnor
{

    class Event;
    class IOHandler;
    class TimerQueue;
    class TimerHandle;
    /**
     * 
     */
    class EventLoop : NonCopyable
    {
        friend class IOHandler;
    public:
        EventLoop();
        ~EventLoop();

        static EventLoop *loopOfCurrentThread();

        void setThreadPools(size_t size);
        void loop();

        //If in the loop thread run immediatly, if not queue the callback in pending, thread safe
        void runInLoop(Functor callback);
        //Put callbacks in the pendingFunctors, thread safe
        void queueInLoop(Functor callback);
        //Put the callback into threadpool to run, thread safe
        void runInPool(Functor callback);

        //Ask the loop to wake up from epoll immediately to deal with pending functors
        void wakeUp();
        //End the loop
        void endLoop();
        //Get the iteration num
        int64_t iteration() { return iteration_; }

        //Return timestamp of poll_wait return
        Timestamp pollReturnTime() { return pollReturnTime_; }

        //Assert that a thread that calls this method is the same thread as the loop
        void assertInLoopThread();

        //handle an IO FD into the loop, are create a handler for it, will take care of its ownership and lifecycle, threadsafe
        std::shared_ptr<IOHandler> handleIO(int fd);
    
        //if has the event
        bool hasIOHandler(std::shared_ptr<IOHandler> handler);

        //Add timer event to the event set
        void addTimer(TimerCallback cb, Timestamp when, double interval);
        //Delete a specific timer
        void removeTimer(TimerHandle id);


        //There are 3 working phases of a loop process: polling, IO handling, and pending handling
        //After ctor and Before calling loop(), it is Ready state,
        //After calling endLoop(), it is end state
        enum LoopState
        {
            Ready,
            Polling,
            IOHandling,
            PendingHandling,
            End
        };

    private:
        //The essential of eventloop
        Epoll poller_;

        std::atomic<bool> quit_;
        const pid_t threadId_;
        //for debug
        uint64_t iteration_;

        LoopState state_;

        //Time when epoll returns
        Timestamp pollReturnTime_;

        //To check if the IO handler is still available in the reactor
        std::set<std::shared_ptr<IOHandler>> IOHandlers_;

        //Real time wakeup pipe, wakeup the loop from epoll to deal with pending Functors
        std::unique_ptr<IOHandler> wakeUpHandler_;

        std::unique_ptr<TimerQueue> timers_;

        //used for protecting pending Functors in mult-threading, change of evenloop data
        //from other threads are only allowed to commit their change into pendingFunctors,
        //And let the loop thread to actually run these changes, In this way we only need to
        //mantain pendingFunctors to be thread-safe
        Mutex pendingFunctorsLock_;
        //priorty map to functor
        std::vector<Functor> pendingFunctors_;

        //To bind for wake up event
        void handleWakeUp();

        bool isLoopThread() { return CurrentThread::tid() == threadId_; }

        //Add a handler to manage lifecycle
        void addIOHandler(std::shared_ptr<IOHandler> handler);
        //modify or remove existing IO event in the epoll
        void updateIOHandler(std::weak_ptr<IOHandler> handler, bool addNew);
    };
} // namespace Hohnor
