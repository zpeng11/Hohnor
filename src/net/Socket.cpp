#include "Socket.h"
#include "Logging.h"
#include <sstream>

using namespace std;
using namespace Hohnor;

std::shared_ptr<struct tcp_info> Socket::getTCPInfo() const
{
    struct tcp_info *tcpi = new struct tcp_info();
    socklen_t len = sizeof(*tcpi);
    memZero(tcpi, len);
    int ret = getsockopt(socketFd_, SOL_TCP, TCP_INFO, tcpi, &len);
    if (ret != 0)
    {
        LOG_SYSERR << "Socket::getTCPInfo error ";
        return nullptr;
    }
    else
    {
        return std::shared_ptr<struct tcp_info>(tcpi);
    }
}

std::shared_ptr<std::string> Socket::getTCPInfoStr() const
{
    auto ti = getTCPInfo();
    if (ti == nullptr)
        return nullptr;
    else
    {
        string *strPtr = new string();
        string &str = *strPtr;
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

        return std::shared_ptr<std::string>(strPtr);
    }
}

std::shared_ptr<std::pair<Socket::SocketFd, InetAddress>> Socket::accept()
{
    auto p =new std::pair<Socket::SocketFd, InetAddress>();
    std::get<0>(*p) = SocketFuncs::accept(socketFd_, reinterpret_cast<sockaddr_in6 *>(&std::get<1>(*p)));
    return std::shared_ptr<std::pair<Socket::SocketFd, InetAddress>>(p);
}