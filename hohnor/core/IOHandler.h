/**
 * IO handler, it handles the lifecycle of a fd, manage how the fd reactively works in event loop. 
 * An IOHandler's lifecycle has 3 phases: Create -> Enabled -> Disabled
 * Created phase is right after constructor in eventloop, give shared ownership to both user and eventloop. 
 * Enabled phase create epoll ctx, loads the fd to epoll, ready for income event.
 * Disabled phase deleted epoll ctx, unload the fd from epoll, do not respond income event, and remove ownership in eventloop.
 */

#pragma once
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/io/FdUtils.h"
#include <memory>
#include <atomic>

namespace Hohnor
{
    class IOHandler;
    struct EpollContext {
        std::weak_ptr<IOHandler> tie;
        int fd;
        EpollContext(std::weak_ptr<IOHandler> tie_, int fd_):tie(tie_),fd(fd_){}
    };

    class EventLoop;
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
        EventLoop *loop_;
        int events_;
        int revents_;
        std::atomic<Status> status_;
        ReadCallback readCallback_;
        WriteCallback writeCallback_;
        CloseCallback closeCallback_;
        ErrorCallback errorCallback_;
        //update EPOLL in the loop
        void update(bool addNew);

        EpollContext * context;

        std::weak_ptr<IOHandler> tie_;
        
        //Run the events according to revents
        void run();

        //Used by event loop to put epoll result back to handler
        void retEvents(int revents) { revents_ = revents; }


    public:
        // Only allow Eventloop to create it
        IOHandler(EventLoop *loop, int fd);
        IOHandler() = delete;
        ~IOHandler();
        //Get current event setting
        int getEvents() { return events_; }
        
        //if events are enabled
        bool enabled() { return status() == Status::Enabled; }
        //Get current status of IOHandler
        Status status() { return status_.load();}
        //Diable all events on this handler from the eventloop, thread safe
        void disable();
        //Enable all events on this handler from the eventloop, thread safe
        void enable();

        void setReadCallback(ReadCallback cb);
        bool hasReadCallback(){ return this->readCallback_ != nullptr; }

        void setWriteCallback(WriteCallback cb);
        bool hasWriteCallback(){ return this->writeCallback_ != nullptr; }

        void setCloseCallback(CloseCallback cb);
        bool hasCloseCallback(){ return this->closeCallback_ != nullptr; }

        void setErrorCallback(ErrorCallback cb);
        bool hasErrorCallback(){ return this->errorCallback_ != nullptr; }

        void tie() { tie_ = shared_from_this(); }

        std::weak_ptr<IOHandler> getTie() const { return tie_; }

    };

} // namespace Hohnor
