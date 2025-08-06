#include "hohnor/net/Socket.h"
#include "hohnor/log/Logging.h"
#include "hohnor/io/FdUtils.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/core/IOHandler.h"
#include <sstream>

using namespace std;
using namespace Hohnor;

Socket::Socket(EventLoop* loop, int family, int type, int protocol)
{
    int fd = SocketFuncs::socket(family, type, protocol);
    socketHandler_ = loop->handleIO(fd);
}

int Socket::fd() const { return socketHandler_->fd(); }

EventLoop * Socket::loop() { return socketHandler_->loop(); }

void Socket::setReadCallback(ReadCallback cb)
{
    socketHandler_->setReadCallback(std::move(cb));
}
void Socket::setWriteCallback(WriteCallback cb)
{
    socketHandler_->setWriteCallback(std::move(cb));
}
void Socket::setCloseCallback(CloseCallback cb)
{
    socketHandler_->setCloseCallback(std::move(cb));
}
void Socket::setErrorCallback(ErrorCallback cb)
{
    socketHandler_->setErrorCallback(std::move(cb));
}

void ListenSocket::bindAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
    InetAddress ina(port, loopbackOnly, ipv6);
    SocketFuncs::bind(fd(), ina.getSockAddr());
}


void ListenSocket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::setReuseAddr error";
    }
}

void ListenSocket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT //Linux support this function since linux3.9
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_SYSERR << "SO_REUSEPORT failed.";
    }
#else
    if (on)
    {
        LOG_ERROR << "SO_REUSEPORT is not supported.";
    }
#endif
}

