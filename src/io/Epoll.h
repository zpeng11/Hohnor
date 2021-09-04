/**
 * Wrap epoll class so it could be easier to use
 */
#pragma once
#include <cstddef> // NULL
#include <memory> //std::unique_ptr
#include "NonCopyable.h"
#include "sys/epoll.h"


namespace Hohnor
{
    class Epoll : NonCopyable
    {
    private:
        int fd_;
        //events for epoll_wait to pass results
        std::unique_ptr<struct epoll_event[]> events_;
        //size of events
        int eventsSize_; 

    public:
        explicit Epoll(int maxEventsSize = 1024, bool closeOnExec = true);
        //epoll_ctl(2) interface
        int ctl(int cmd, int fd, struct epoll_event *event);
        int add(int fd, int events, void *ptr = NULL);
        int modify(int fd, int events, void *ptr = NULL);
        int remove(int fd);
        int wait(int timeout);
        ~Epoll();
    };

} // namespace Hohnor
