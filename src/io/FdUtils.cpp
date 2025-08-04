#include "hohnor/io/FdUtils.h"
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include "hohnor/log/Logging.h"

namespace Hohnor
{
    namespace FdUtils
    {
    } // namespace FdUtils
} // namespace Hohnor

using namespace Hohnor;
using namespace Hohnor::FdUtils;

int Hohnor::FdUtils::setNonBlocking(int fd, bool nonBlocking)
{
    int oldOptions = fcntl(fd, F_GETFL);
    if (oldOptions == -1)
    {
        LOG_SYSERR << "FdUtils::setNonBlocking error";
        return -1;
    }
    int newOptions = nonBlocking ? (oldOptions | O_NONBLOCK) : (oldOptions & ~(O_NONBLOCK));
    int retVal = fcntl(fd, F_SETFL, newOptions);
    if (retVal == -1)
    {
        LOG_SYSERR << "FdUtils::setNonBlocking error";
        return -1;
    }
    return oldOptions;
}

int FdUtils::setCloseOnExec(int fd, bool closeOnExec)
{
    int oldOptions = fcntl(fd, F_GETFD);
    if (oldOptions == -1)
    {
        LOG_SYSERR << "FdUtils::setCloseOnExec error (F_GETFD)";
        return -1;
    }

    int newOptions = closeOnExec ? (oldOptions | FD_CLOEXEC)
                                 : (oldOptions & ~FD_CLOEXEC);

    int retVal = fcntl(fd, F_SETFD, newOptions);
    if (retVal == -1)
    {
        LOG_SYSERR << "FdUtils::setCloseOnExec error (F_SETFD)";
        return -1;
    }

    return oldOptions;
}


void FdUtils::close(int fd)
{
    if (::close(fd) < 0)
    {
        LOG_SYSERR << "close " << fd << " error";
    }
}

static bool is_fd_in_procfs(int fd) {
    std::string path = "/proc/self/fd/" + std::to_string(fd);
    return access(path.c_str(), F_OK) == 0;
}

void FdGuard::setFd(int fd)
{
    HCHECK(is_fd_in_procfs(fd)) << "The fd trying to guard is not open to the process yet";
    fd_ = fd;
}