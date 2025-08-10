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
            auto pair = self->accept();
            cb(std::move(pair.first), std::move(pair.second));
        }
    });
}

std::pair<std::shared_ptr<IOHandler>, InetAddress> TCPAcceptor::accept()
{
    std::pair<std::shared_ptr<IOHandler>, InetAddress> pair{nullptr, InetAddress()};
    int acceptedFd = SocketFuncs::accept(this->fd(), reinterpret_cast<sockaddr_in6 *>(&pair.second));
    if (acceptedFd < 0)
    {
        pair.second = InetAddress();
        return pair;
    }
    pair.first = this->loop()->handleIO(acceptedFd);
    return pair;
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