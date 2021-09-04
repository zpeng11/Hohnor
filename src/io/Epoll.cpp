#include "Epoll.h"
#include "FdUtils.h"
#include <sys/epoll.h>

using namespace Hohnor;

Epoll::Epoll(size_t maxEventsSize, bool closeOnExec)
    : events_(std::move(std::unique_ptr<Event[]>(new Event[maxEventsSize]))), maxEventsSize_(maxEventsSize), readyEvents_(0)
{
    fd_ = epoll_create1(closeOnExec ? EPOLL_CLOEXEC : 0);
}

Epoll::~Epoll()
{
    FdUtils::close(fd_);
}

int Epoll::ctl(int cmd, int fd, Event *event)
{
    int ret = epoll_ctl(fd_, cmd, fd, event);
    if (ret < 0)
    {
        LOG_SYSERR << "Hohor::Epoll:ctl() error";
    }
    return ret;
}

int Epoll::add(int fd, int trackEvents, void *ptr)
{
    Event e;
    e.events = trackEvents;
    if (ptr != NULL)
        e.data.ptr = ptr;
    else
        e.data.fd = fd;
    return this->ctl(EPOLL_CTL_ADD, fd, &e);
}

int Epoll::modify(int fd, int trackEvents, void *ptr)
{
    Event e;
    e.events = trackEvents;
    if (ptr != NULL)
        e.data.ptr = ptr;
    else
        e.data.fd = fd;
    return this->ctl(EPOLL_CTL_MOD, fd, &e);
}

int Epoll::remove(int fd)
{
    return this->ctl(EPOLL_CTL_DEL, fd, NULL);
}

int Epoll::wait(int timeout, const sigset_t *sigmask)
{
    int ret;
    if (sigmask == NULL)
        ret = epoll_wait(fd_, events_.get(), maxEventsSize_, timeout);
    else
        ret = epoll_pwait(fd_, events_.get(), maxEventsSize_, timeout, sigmask);
    if (ret == -1)
        LOG_SYSERR << "Hohnor::Epoll::wait() ";
    readyEvents_ = ret;
    return ret;
}