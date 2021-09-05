#include "TCPSocket.h"
#include "Logging.h"

using namespace Hohnor;
std::shared_ptr<struct tcp_info> TCPListenSocket::getTCPInfo() const
{
    struct tcp_info *tcpi = new struct tcp_info();
    socklen_t len = sizeof(*tcpi);
    memZero(tcpi, len);
    int ret = getsockopt(this->fd(), SOL_TCP, TCP_INFO, tcpi, &len);
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

std::string TCPListenSocket::getTCPInfoStr() const
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

SocketAddrPair TCPListenSocket::accept()
{
    SocketAddrPair pair;
    pair.fd() = SocketFuncs::accept(fd(),
                                    reinterpret_cast<sockaddr_in6 *>(pair.pair_.second.get()));
    if (pair.fd() < 0)
    {
        std::get<1>(pair.pair_) = nullptr;
        return pair;
    }
    return pair;
}

void TCPListenSocket::setTCPNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), IPPROTO_TCP, TCP_NODELAY,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::setTcpNoDelay error";
    }
}

void TCPListenSocket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), SOL_SOCKET, SO_KEEPALIVE,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::setKeepAlive error";
    }
}