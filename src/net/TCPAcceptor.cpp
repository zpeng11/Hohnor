#include "hohnor/net/TCPAcceptor.h"
#include "hohnor/log/Logging.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/core/EventLoop.h"

using namespace Hohnor;
struct tcp_info TCPAcceptor::getTCPInfo() const
{
    struct tcp_info tcpi;
    socklen_t len = sizeof(tcpi);
    memZero(&tcpi, len);
    int ret = getsockopt(this->fd(), SOL_TCP, TCP_INFO, &tcpi, &len);
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::getTCPInfo error ";
    }
    return tcpi;
}

std::string TCPAcceptor::getTCPInfoStr() const
{
    struct tcp_info ti = getTCPInfo();
    string str;
    const char *percentU = "%u";
    str.append("unrecovered=").append(Fmt(percentU, ti.tcpi_retransmits).data()); // Number of unrecovered [RTO] timeouts
    str.append(" rto=").append(Fmt(percentU, ti.tcpi_rto).data());                // Retransmit timeout in usec
    str.append(" ato=").append(Fmt(percentU, ti.tcpi_ato).data());                // Predicted tick of soft clock in usec
    str.append(" snd_mss=").append(Fmt(percentU, ti.tcpi_snd_mss).data());
    str.append(" rcv_mss=").append(Fmt(percentU, ti.tcpi_rcv_mss).data());
    str.append(" lost=").append(Fmt(percentU, ti.tcpi_lost).data());       // Lost packets
    str.append(" retrans=").append(Fmt(percentU, ti.tcpi_retrans).data()); // Retransmitted packets out
    str.append(" rtt=").append(Fmt(percentU, ti.tcpi_rtt).data());         // Smoothed round trip time in usec
    str.append(" rttvar=").append(Fmt(percentU, ti.tcpi_rttvar).data());   // Medium deviation
    str.append(" ssthresh=").append(Fmt(percentU, ti.tcpi_snd_ssthresh).data());
    str.append(" cwnd=").append(Fmt(percentU, ti.tcpi_snd_cwnd).data());
    str.append(" total_retrans=").append(Fmt(percentU, ti.tcpi_total_retrans).data()); // Total retransmits for entire connection

    return std::move(str);
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