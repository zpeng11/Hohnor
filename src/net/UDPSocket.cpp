#include "hohnor/net/UDPSocket.h"
#include "hohnor/log/Logging.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/core/EventLoop.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace Hohnor;

ssize_t UDPSocket::sendTo(const void* data, size_t len, const InetAddress& addr)
{
    return ::sendto(fd(), data, len, 0, 
                   reinterpret_cast<const struct sockaddr*>(addr.getSockAddr()), 
                   addr.getSockLen());
}

ssize_t UDPSocket::recvFrom(void* buffer, size_t len, InetAddress& fromAddr)
{
    struct sockaddr_in6 addr;
    socklen_t addrLen = sizeof(addr);
    ssize_t n = ::recvfrom(fd(), buffer, len, 0, 
                          reinterpret_cast<struct sockaddr*>(&addr), &addrLen);
    if (n >= 0)
    {
        fromAddr = InetAddress(addr);
    }
    return n;
}

ssize_t UDPSocket::send(const void* data, size_t len)
{
    return ::send(fd(), data, len, 0);
}

ssize_t UDPSocket::recv(void* buffer, size_t len)
{
    return ::recv(fd(), buffer, len, 0);
}

void UDPSocket::setBroadcast(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), SOL_SOCKET, SO_BROADCAST,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "UDPSocket::setBroadcast error";
    }
}

void UDPSocket::setMulticastTTL(int ttl)
{
    int ret = ::setsockopt(fd(), IPPROTO_IP, IP_MULTICAST_TTL,
                           &ttl, static_cast<socklen_t>(sizeof ttl));
    if (ret != 0)
    {
        LOG_SYSERR << "UDPSocket::setMulticastTTL error";
    }
}

void UDPSocket::joinMulticastGroup(const InetAddress& groupAddr, const InetAddress& interfaceAddr)
{
    if (groupAddr.family() == AF_INET)
    {
        struct ip_mreq mreq;
        mreq.imr_multiaddr = groupAddr.getSockAddr4()->sin_addr;
        if (interfaceAddr.isValid())
        {
            mreq.imr_interface = interfaceAddr.getSockAddr4()->sin_addr;
        }
        else
        {
            mreq.imr_interface.s_addr = INADDR_ANY;
        }
        
        int ret = ::setsockopt(fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP,
                               &mreq, static_cast<socklen_t>(sizeof mreq));
        if (ret != 0)
        {
            LOG_SYSERR << "UDPSocket::joinMulticastGroup error";
        }
    }
    else if (groupAddr.family() == AF_INET6)
    {
        struct ipv6_mreq mreq6;
        mreq6.ipv6mr_multiaddr = groupAddr.getSockAddr6()->sin6_addr;
        mreq6.ipv6mr_interface = 0; // Use default interface
        
        int ret = ::setsockopt(fd(), IPPROTO_IPV6, IPV6_JOIN_GROUP,
                               &mreq6, static_cast<socklen_t>(sizeof mreq6));
        if (ret != 0)
        {
            LOG_SYSERR << "UDPSocket::joinMulticastGroup (IPv6) error";
        }
    }
}

void UDPSocket::leaveMulticastGroup(const InetAddress& groupAddr, const InetAddress& interfaceAddr)
{
    if (groupAddr.family() == AF_INET)
    {
        struct ip_mreq mreq;
        mreq.imr_multiaddr = groupAddr.getSockAddr4()->sin_addr;
        if (interfaceAddr.isValid())
        {
            mreq.imr_interface = interfaceAddr.getSockAddr4()->sin_addr;
        }
        else
        {
            mreq.imr_interface.s_addr = INADDR_ANY;
        }
        
        int ret = ::setsockopt(fd(), IPPROTO_IP, IP_DROP_MEMBERSHIP,
                               &mreq, static_cast<socklen_t>(sizeof mreq));
        if (ret != 0)
        {
            LOG_SYSERR << "UDPSocket::leaveMulticastGroup error";
        }
    }
    else if (groupAddr.family() == AF_INET6)
    {
        struct ipv6_mreq mreq6;
        mreq6.ipv6mr_multiaddr = groupAddr.getSockAddr6()->sin6_addr;
        mreq6.ipv6mr_interface = 0; // Use default interface
        
        int ret = ::setsockopt(fd(), IPPROTO_IPV6, IPV6_LEAVE_GROUP,
                               &mreq6, static_cast<socklen_t>(sizeof mreq6));
        if (ret != 0)
        {
            LOG_SYSERR << "UDPSocket::leaveMulticastGroup (IPv6) error";
        }
    }
}

void UDPSocket::setMulticastLoopback(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), IPPROTO_IP, IP_MULTICAST_LOOP,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "UDPSocket::setMulticastLoopback error";
    }
}

void UDPSocket::setRecvBufferSize(int size)
{
    int ret = ::setsockopt(fd(), SOL_SOCKET, SO_RCVBUF,
                           &size, static_cast<socklen_t>(sizeof size));
    if (ret != 0)
    {
        LOG_SYSERR << "UDPSocket::setRecvBufferSize error";
    }
}

void UDPSocket::setSendBufferSize(int size)
{
    int ret = ::setsockopt(fd(), SOL_SOCKET, SO_SNDBUF,
                           &size, static_cast<socklen_t>(sizeof size));
    if (ret != 0)
    {
        LOG_SYSERR << "UDPSocket::setSendBufferSize error";
    }
}

int UDPSocket::getRecvBufferSize() const
{
    int size = 0;
    socklen_t len = sizeof(size);
    int ret = ::getsockopt(fd(), SOL_SOCKET, SO_RCVBUF,
                           &size, &len);
    if (ret != 0)
    {
        LOG_SYSERR << "UDPSocket::getRecvBufferSize error";
        return -1;
    }
    return size;
}

int UDPSocket::getSendBufferSize() const
{
    int size = 0;
    socklen_t len = sizeof(size);
    int ret = ::getsockopt(fd(), SOL_SOCKET, SO_SNDBUF,
                           &size, &len);
    if (ret != 0)
    {
        LOG_SYSERR << "UDPSocket::getSendBufferSize error";
        return -1;
    }
    return size;
}

void UDPListenSocket::bindAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
    InetAddress ina(port, loopbackOnly, ipv6);
    SocketFuncs::bind(fd(), ina.getSockAddr());
}

void UDPListenSocket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret != 0)
    {
        LOG_SYSERR << "UDPListenSocket::setReuseAddr error";
    }
}

void UDPListenSocket::setReusePort(bool on)
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