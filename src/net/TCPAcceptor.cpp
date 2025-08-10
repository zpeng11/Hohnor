#include "hohnor/net/TCPAcceptor.h"
#include "hohnor/log/Logging.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/core/EventLoop.h"

using namespace Hohnor;
struct tcp_info TCPAcceptor::getTCPInfo() const
{
    return SocketFuncs::getTCPInfo(this->fd());
}

std::string TCPAcceptor::getTCPInfoStr() const
{
    return SocketFuncs::getTCPInfoStr(this->fd());
}

void TCPAcceptor::setAcceptCallback(AcceptCallback cb)
{
    std::weak_ptr<TCPAcceptor> weakSelf = shared_from_this();
    this->setReadCallback([weakSelf, cb]() {
        auto self = weakSelf.lock();
        if (self)
        {
            auto handler = self->accept();
            cb(std::make_shared<TCPConnection>(handler));
        }
    });
}

std::shared_ptr<IOHandler> TCPAcceptor::accept()
{
    std::shared_ptr<IOHandler> handler = nullptr;
    InetAddress addr;
    int acceptedFd = SocketFuncs::accept(this->fd(), reinterpret_cast<sockaddr_in6 *>(&addr));
    if (acceptedFd < 0)
    {
        return handler;
    }
    handler = this->loop()->handleIO(acceptedFd);
    return handler;
}

void TCPAcceptor::setTCPNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), IPPROTO_TCP, TCP_NODELAY,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::setTcpNoDelay error";
    }
}

void TCPAcceptor::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), SOL_SOCKET, SO_KEEPALIVE,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::setKeepAlive error";
    }
}