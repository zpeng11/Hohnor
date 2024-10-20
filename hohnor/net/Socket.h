/**
 * Class wrapper for sockets, builds on SocketWrap.h
 */
#pragma once
#include "common/Types.h"
#include "SocketWrap.h"
#include "InetAddress.h"
#include "common/NonCopyable.h"
#include "common/Copyable.h"
#include "FdUtils.h"
#include <netinet/tcp.h>
#include <memory>
#include <map>

namespace Hohnor
{
    class Socket : public FdGuard
    {
    public:
        typedef int SocketFd;
        //Initilize from existing fd
        Socket(SocketFd socketFd) : FdGuard(socketFd) {}
        //Initilize with paramters, in case failed, !!!abort the program
        Socket(int family, int type, int protocol = 0) : FdGuard(SocketFuncs::socket(family, type, protocol)) {}

        //For clent connect
        void connect(const InetAddress &addr) { SocketFuncs::connect(fd(), addr.getSockAddr()); }

        //Socket error
        int getSockError() { return SocketFuncs::getSocketError(fd()); }
        //Get socket error string
        string getSockErrorStr() { return strerror_tl(getSockError()); }

        InetAddress getLocalAddr() { return InetAddress(SocketFuncs::getLocalAddr(fd())) ; }
        InetAddress getPeerAddr() { return InetAddress(SocketFuncs::getPeerAddr(fd())) ; }
        bool isSelfConnect() { return SocketFuncs::isSelfConnect(fd()); }

        ~Socket() = default;
    };

    inline std::shared_ptr<Socket> socketSharedManage(Socket::SocketFd fd)
    {
        return std::shared_ptr<Socket>(new Socket(fd));
    }

    class ListenSocket : public Socket
    {
    private:
        void connect(const InetAddress &addr);
        InetAddress getLocalAddr();
        InetAddress getPeerAddr();
        bool isSelfConnect();

    public:
        ListenSocket(Socket::SocketFd fd) : Socket(fd) {}
        ListenSocket(int family, int type, int protocol = 0)
            : Socket(family, type, protocol) {}

        //Bind the address. In case failed, !!!abort the program
        void bindAddress(const InetAddress &localaddr) { SocketFuncs::bind(fd(), localaddr.getSockAddr()); }

        //Bind the address. In case failed, !!!abort the program
        void bindAddress(uint16_t port, bool loopbackOnly = false, bool ipv6 = false);

        //listen to the current socket. In case failed, !!!abort the program
        void listen() { SocketFuncs::listen(fd()); }

        // Enable/disable SO_REUSEADDR
        void setReuseAddr(bool on);

        // Enable/disable SO_REUSEPORT
        void setReusePort(bool on);

        ~ListenSocket() = default;
    };
} // namespace Hohnor
