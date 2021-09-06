#include "SignalHandler.h"
#include "Logging.h"
#include <sys/socket.h>

namespace Hohnor
{
    void sysSigHandle(int sig)
    {
        LOG_WARN<<"Enter handler";
        int save_errno = errno;
        int msg = sig;
        int ret = send(SignalHandler::getInst().pipefd_[1], (char *)&msg, 1, 0);
        if (ret <= 0)
            LOG_SYSERR << "Handling signal and putting message into pipe error";
        errno = save_errno;
    }
} // namespace Hohnor

using namespace Hohnor;

SignalHandler::SignalHandler()
{
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd_);
    if (ret < 0)
        LOG_SYSERR << "SignalHandler::SignalHandler() socketpair creation error";
    //Nonblocking write so that writing to pipe would not be blocked(signal handle should return fast!)
    FdUtils::setNonBlocking(pipefd_[1]);
    //close on exec so that different process doesnot share same pipe(processes should have own SignalHandler)
    FdUtils::setCloseOnExec(pipefd_[0]);
    FdUtils::setCloseOnExec(pipefd_[1]);
    memZero(signals_, 512);
}

SignalHandler &SignalHandler::getInst()
{
    //For c++11, this is a thread-safe operation on linux
    static SignalHandler instance;
    return instance;
}

SignalHandler::~SignalHandler()
{
    FdUtils::close(pipefd_[0]);
    FdUtils::close(pipefd_[1]);
}

SignalHandler::Iter SignalHandler::receive()
{
    readySignals_ = ::recv(pipefd_[0], signals_, 512, 0);
    if (readySignals_ == -1)
    {
        LOG_SYSERR << "SignalHandler::receive() Handler recieve error";
    }
    else if (readySignals_ == 0 && errno != EINTR)
    {
        LOG_ERROR << "SignalHandler::receive() Handler recieve error, writing end may be closed";
    }
    return Iter(this);
}

void SignalHandler::addSig(int signal)
{
    struct sigaction sa;
    memZero(&sa, sizeof sa);
    sa.sa_handler = sysSigHandle;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    int ret = sigaction(signal, &sa, NULL);
    if (ret < 0)
        LOG_SYSERR << "Signal action settle failed";
}
