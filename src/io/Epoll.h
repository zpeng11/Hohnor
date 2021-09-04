/**
 * Wrap epoll class so it could be easier to use
 */
#pragma once
#include <cstddef> // NULL
#include <memory>  //std::unique_ptr
#include <sys/epoll.h>
#include "NonCopyable.h"
#include "Logging.h"

namespace Hohnor
{
    class Epoll : NonCopyable
    {
    private:
        typedef struct epoll_event Event;
        int fd_;
        //events for epoll_wait to pass results
        std::unique_ptr<Event[]> events_;
        //max size of events
        size_t maxEventsSize_;
        //ready events after epoll_wait
        size_t readyEvents_;
        class Iter
        {
        private:
            Epoll *ptr_;
            size_t position_;

        public:
            Iter(Epoll *ptr) : ptr_(ptr), position_(0) {}
            bool hasNext() { return position_ >= ptr_->readyEvents_; }
            Event& next()
            {
                CHECK(position_ < ptr_->readyEvents_) << " Hohnor::Epoll::Iter::next() error";
                return ptr_->events_[position_++];
            }
            ~Iter() { ptr_->readyEvents_ = 0; }
        };

    public:
        explicit Epoll(size_t maxEventsSize = 1024, bool closeOnExec = true);
        int fd() { return fd_; }
        //epoll_ctl(2) interface
        int ctl(int cmd, int fd, Event *event);
        //add fd to the RB tree, by default specify data as fd itself, if ptr is specified, then use the ptr
        int add(int fd, int trackEvents, void *ptr = NULL);
        //same as add interface but only modify existing fd in the RB tree
        int modify(int fd, int trackEvents, void *ptr = NULL);
        //remove a fd from RB tree
        int remove(int fd);

        int wait(int timeout = -1, const sigset_t *sigmask = NULL);
        Iter results() { return Iter(this); }
        ~Epoll();
    };

} // namespace Hohnor
