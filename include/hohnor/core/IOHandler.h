

#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/io/FdUtils.h"
#include <memory>
#include <atomic>

namespace Hohnor
{
    class IOHandler;
    class EventLoop;
    class TimerQueue; 

    /**
    * IO handler, it handles the lifecycle of a fd, manage how the fd reactively works in event loop. 
    * An IOHandler's lifecycle has 3 phases: Create -> Enabled -> Disabled
    * Created phase is right after constructor in eventloop, give shared ownership to both user and eventloop. 
    * Enabled phase create epoll ctx, loads the fd to epoll, ready for income event. Most of its lifecycle, it should be enabled.
    * Disabled phase deleted epoll ctx, unload the fd from epoll, do not respond income event, and remove ownership in eventloop.
    * Please notice that IOHandler's lifecycle should be shorter than EventLoop's lifecycle, so that it can be safely deleted when eventloop is deleted.
    */
    class IOHandler: public FdGuard, public std::enable_shared_from_this<IOHandler> 
    {
        friend class EventLoop;
    public:
        enum class Status {
            Created,
            Enabled,
            Disabled
        };

    private:
        //do not manage life cycle of this fd
        std::shared_ptr<EventLoop> loop_;
        int events_;
        int revents_;
        Status status_;
        ReadCallback readCallback_;
        WriteCallback writeCallback_;
        CloseCallback closeCallback_;
        ErrorCallback errorCallback_;
        //update EPOLL in the loop
        void updateInLoop(std::shared_ptr<IOHandler> handler, Status nextStatus);
        void update(Status nextStatus);
        
        //Run the events according to revents
        void run();

        //Used by event loop to put epoll result back to handler
        void retEvents(int revents) { revents_ = revents; }

    protected:
        // Constructor, hinden so that only eventloop can call
        IOHandler(std::shared_ptr<EventLoop> loop, int fd);
    public:
        // Delete default constructor
        IOHandler() = delete;
        ~IOHandler();
        //Get current event setting, this is not thread safe
        int getEvents() { return events_; }
        
        //if events are enabled, this is not thread safe
        bool inline isEnabled() { return status() == Status::Enabled; }
        //Get current status of IOHandler, this is not thread safe
        Status inline status() { return status_;}
        //Diable all events and reset callbacks on this handler from the eventloop, thread safe
        void disable();
        //Enable all events on this handler from the eventloop, thread safe
        void enable();
        //thread safe
        void setReadCallback(ReadCallback cb);
        //thread safe
        void setWriteCallback(WriteCallback cb);
        //thread safe
        void setCloseCallback(CloseCallback cb);
        //thread safe
        void setErrorCallback(ErrorCallback cb);

        void setReadEvent(bool on);
        void setWriteEvent(bool on);
        void setCloseEvent(bool on);
        void setErrorEvent(bool on);

        void cleanCallbacks()
        {
            readCallback_ = nullptr;
            writeCallback_ = nullptr;
            closeCallback_ = nullptr;
            errorCallback_ = nullptr;
        }

        //Get the loop that manages this handler
        std::shared_ptr<EventLoop> loop() { return loop_; }
    };

} // namespace Hohnor
