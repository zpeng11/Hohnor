#include "SocketWrap.h"
#include "Logging.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h> // snprintf
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

using namespace Hohnor;
using namespace SocketFuncs;

SocketFd SocketFuncs::socket(int __family, int __type, int __protocol)
{
    int sockfd = ::socket(__family, __type, __protocol);
    if (sockfd < 0)
    {
        LOG_SYSFATAL << "SocketFuncs::socket creation error " << strerror_tl(errno);
    }
    return sockfd;
}

SocketFd SocketFuncs::nonBlockingSocket(sa_family_t family)
{
    return socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
}

void SocketFuncs::bind(SocketFd socketfd, const struct sockaddr *addr)
{
    int ret = ::bind(socketfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0)
    {
        LOG_SYSFATAL << "SocketFuncs::bind " << " error " << strerror_tl(errno);
    }
}

void SocketFuncs::listen(SocketFd socketfd)
{
    int ret = ::listen(socketfd, SOMAXCONN);
    if (ret < 0)
    {
        LOG_SYSFATAL << "SocketFuncs::listen " << socketfd << " error" << strerror_tl(errno);
    }
}

SocketFd SocketFuncs::accept(int socketfd, struct sockaddr_in6 *addr)
{
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
    int connfd = ::accept4(socketfd, sockaddr_cast(addr),
                           &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0)
    {
        int savedErrno = errno;
        LOG_SYSERR << "SocketFuncs::accept error";
        switch (savedErrno)
        {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO: // ???
        case EPERM:
        case EMFILE: // per-process lmit of open file desctiptor ???
            // expected errors
            errno = savedErrno;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            // unexpected errors
            LOG_FATAL << "unexpected error of ::accept " << strerror_tl(savedErrno);
            break;
        default:
            LOG_FATAL << "unknown error of ::accept " << strerror_tl(savedErrno);
            break;
        }
    }
    return connfd;
}

void SocketFuncs::close(SocketFd socketfd)
{
    if (::close(socketfd) < 0)
    {
        LOG_SYSERR << "SocketFuncs::close " << socketfd << " error";
    }
}

void SocketFuncs::shutdownWrite(SocketFd socketfd)
{
    if (::shutdown(socketfd, SHUT_WR) < 0)
    {
        LOG_SYSERR << "SocketFuncs::shutdownWrite " << socketfd << " error";
    }
}

string SocketFuncs::toIpPort(const struct sockaddr *addr)
{
    string str;
    uint16_t port = 0;
    if (addr->sa_family == AF_INET6)
    {
        str += '[';
        str += toIp(addr);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        port = networkToHost16(addr6->sin6_port);
        str += ']';
    }
    else
    {
        str = std::move(toIp(addr));
        const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
        port = networkToHost16(addr4->sin_port);
    }
    str += ':';
    str += std::to_string(port);
    return std::move(str);
}

string SocketFuncs::toIp(const struct sockaddr *addr)
{
    int size = 256;
    char buf[256];
    memZero(buf, size);
    if (addr->sa_family == AF_INET)
    {
        CHECK(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    else if (addr->sa_family == AF_INET6)
    {
        CHECK(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
    return buf;
}

void SocketFuncs::fromIpPort(StringPiece ip, uint16_t port,
                             struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip.data(), &addr->sin_addr) <= 0)
    {
        LOG_SYSERR << "SocketFuncs::fromIpPort error";
    }
}

void SocketFuncs::fromIpPort(StringPiece ip, uint16_t port,
                             struct sockaddr_in6 *addr)
{
    addr->sin6_family = AF_INET6;
    addr->sin6_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET6, ip.data(), &addr->sin6_addr) <= 0)
    {
        LOG_SYSERR << "SocketFuncs::fromIpPort error";
    }
}

int SocketFuncs::getSocketError(int socketfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

void SocketFuncs::connect(SocketFd socketfd, const struct sockaddr *addr)
{
    int retVal = connect(socketfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (retVal < 0)
    {
        LOG_SYSERR << "SocketFuncs::connect error " ;
    }
}

struct sockaddr_in6 SocketFuncs::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localaddr;
    memZero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
    {
        LOG_SYSERR << "SocketFuncs::getLocalAddr error ";
    }
    return localaddr;
}

struct sockaddr_in6 SocketFuncs::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peeraddr;
    memZero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
    {
        LOG_SYSERR << "SocketFuncs::getPeerAddr error ";
    }
    return peeraddr;
}

bool SocketFuncs::isSelfConnect(int sockfd)
{
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET)
    {
        const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in *>(&localaddr);
        const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in *>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if (localaddr.sin6_family == AF_INET6)
    {
        return localaddr.sin6_port == peeraddr.sin6_port && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
    }
    else
    {
        return false;
    }
}