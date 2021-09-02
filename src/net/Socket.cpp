#include "Socket.h"
#include "Logging.h"
#include <sstream>

using namespace std;
using namespace Hohnor;

void Socket::bindAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
    InetAddress ina(port, loopbackOnly, ipv6);
    SocketFuncs::bind(socketFd_, ina.getSockAddr());
}

std::shared_ptr<struct tcp_info> Socket::getTCPInfo() const
{
    struct tcp_info *tcpi = new struct tcp_info();
    socklen_t len = sizeof(*tcpi);
    memZero(tcpi, len);
    int ret = getsockopt(socketFd_, SOL_TCP, TCP_INFO, tcpi, &len);
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::getTCPInfo error ";
        delete tcpi;
        return nullptr;
    }
    else
    {
        return std::shared_ptr<struct tcp_info>(tcpi);
    }
}

std::string Socket::getTCPInfoStr() const
{
    auto ti = getTCPInfo();
    if (ti == nullptr)
        return "";
    else
    {
        string str;
        const char *percentU = "%u";
        str.append("unrecovered=").append(Fmt(percentU, ti->tcpi_retransmits).data()); // Number of unrecovered [RTO] timeouts
        str.append(" rto=").append(Fmt(percentU, ti->tcpi_rto).data());                // Retransmit timeout in usec
        str.append(" ato=").append(Fmt(percentU, ti->tcpi_ato).data());                // Predicted tick of soft clock in usec
        str.append(" snd_mss=").append(Fmt(percentU, ti->tcpi_snd_mss).data());
        str.append(" rcv_mss=").append(Fmt(percentU, ti->tcpi_rcv_mss).data());
        str.append(" lost=").append(Fmt(percentU, ti->tcpi_lost).data());       // Lost packets
        str.append(" retrans=").append(Fmt(percentU, ti->tcpi_retrans).data()); // Retransmitted packets out
        str.append(" rtt=").append(Fmt(percentU, ti->tcpi_rtt).data());         // Smoothed round trip time in usec
        str.append(" rttvar=").append(Fmt(percentU, ti->tcpi_rttvar).data());   // Medium deviation
        str.append(" ssthresh=").append(Fmt(percentU, ti->tcpi_snd_ssthresh).data());
        str.append(" cwnd=").append(Fmt(percentU, ti->tcpi_snd_cwnd).data());
        str.append(" total_retrans=").append(Fmt(percentU, ti->tcpi_total_retrans).data()); // Total retransmits for entire connection

        return std::move(str);
    }
}

std::shared_ptr<SocketAddrPair> Socket::accept()
{
    InetAddress ina;
    int ret = SocketFuncs::accept(socketFd_, reinterpret_cast<sockaddr_in6 *>(&ina));
    if (ret < 0)
    {
        LOG_SYSERR << "Socket::accept error ";
        return nullptr;
    }
    else
    {
        SocketAddrPair * p = new SocketAddrPair(ret,ina);
        return std::shared_ptr<SocketAddrPair>(p);
    }
}

void Socket::setTCPNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(socketFd_, IPPROTO_TCP, TCP_NODELAY,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::setTcpNoDelay error";
    }
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::setReuseAddr error";
    }
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT //Linux support this function since linux3.9
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(socketFd_, SOL_SOCKET, SO_REUSEPORT,
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

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(socketFd_, SOL_SOCKET, SO_KEEPALIVE,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::setKeepAlive error";
    }
}