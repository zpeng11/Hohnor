#include "Epoll.h"
#include "FdUtils.h"
#include <sys/epoll.h>


using namespace Hohnor;

Epoll::Epoll(int maxEventsSize, bool closeOnExec)
{
    fd_ = epoll_create1(closeOnExec? EPOLL_CLOEXEC:0);
}

Epoll::~Epoll()
{
    FdUtils::close(fd_);
}