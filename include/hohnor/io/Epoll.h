/**
 * Wrap epoll class so it could be easier to use
 */
#pragma once
#include <cstddef> // NULL
#include <memory>  //std::unique_ptr
#include <sys/epoll.h>
#include "hohnor/common/NonCopyable.h"
#include "hohnor/log/Logging.h"
#include "hohnor/io/FdUtils.h"

namespace Hohnor
{
    /**
     * Wrapper for ::epoll(2) in linux, 
     * kindly reminder that ctl Ops are safe while another thread is waiting.
     */
    class Epoll : public FdGuard  //Inherited as FD holder class
    {
    private:
        typedef struct epoll_event epoll_event;
        //events for epoll_wait to pass results
        std::unique_ptr<epoll_event[]> events_;
        //max size of events
        size_t maxEventsSize_;
        //ready events after epoll_wait
        size_t readyEvents_;
        class Iter
        {
        private:
            Epoll *ptr_;
            ssize_t position_;

        public:
            Iter(Epoll *ptr) : ptr_(ptr), position_(0) {}
            bool hasNext() { return position_ < size(); }
            ssize_t size() { return ptr_->readyEvents_; }
            epoll_event &next()
            {
                HCHECK(position_ < ptr_->readyEvents_) << " Hohnor::Epoll::Iter::next() error";
                return ptr_->events_[position_++];
            }
            ~Iter() { ptr_->readyEvents_ = 0; }
        };

    public:
        explicit Epoll(size_t maxEventsSize = 1024, bool closeOnExec = true);
        //epoll_ctl(2) interface
        int ctl(int cmd, int fd, epoll_event *event);
        //add fd to the RB tree, by default specify data as fd itself, if ptr is specified, then use the ptr
        int add(int fd, int trackEvents, void *ptr = NULL);
        //same as add interface but only modify existing fd in the RB tree
        int modify(int fd, int trackEvents, void *ptr = NULL);
        //remove a fd from RB tree
        int remove(int fd);

        Iter wait(int timeout = -1, const sigset_t *sigmask = NULL);
        ~Epoll() = default;
    };

} // namespace Hohnor
