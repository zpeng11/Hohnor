#include "FdUtils.h"
#include <fcntl.h>
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
        LOG_SYSERR << "FdUtils::setCloseOnExec error";
        return -1;
    }
    int newOptions = closeOnExec ? (oldOptions | O_CLOEXEC) : (oldOptions & ~(O_CLOEXEC));
    int retVal = fcntl(fd, F_SETFD, newOptions);
    if (retVal == -1)
    {
        LOG_SYSERR << "FdUtils::setCloseOnExec error";
        return -1;
    }
    return oldOptions;
}

void FdUtils::close(int socketfd)
{
    if (::close(socketfd) < 0)
    {
        LOG_SYSERR << "SocketFuncs::close " << socketfd << " error";
    }
}