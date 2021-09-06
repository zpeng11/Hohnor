#include "SignalHandler.h"
#include "Logging.h"
#include <sys/socket.h>

using namespace Hohnor;

SignalHandler::SignalHandler()
{
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd_);
    if (ret < 0)
        LOG_SYSERR << "SignalHandler::SignalHandler() socketpair creation error";
    FdUtils::setNonBlocking(pipefd_[1]);
    memZero(signals_,512);
}

SignalHandler& SignalHandler::getInst()
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
    ret_ = ::recv(pipefd_[0], signals_, 512, 0);
    if(ret_ == -1)
    {
        LOG_SYSERR<<"SignalHandler::receive() Handler recieve error";
    }
    else if(ret_ == 0 && errno != EINTR)
    {
        LOG_ERROR<<"SignalHandler::receive() Handler recieve error, writing end may be closed";
    }
    return Iter(this);
}