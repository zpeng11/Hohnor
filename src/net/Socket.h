/**
 * Class wrapper for sockets, builds on SocketWrap.h
 */
#pragma once
#include "Types.h"
#include "SocketWrap.h"
#include "InetAddress.h"
#include "NonCopyable.h"
#include "Copyable.h"
#include "FdUtils.h"
#include <netinet/tcp.h>
#include <memory>
#include <map>

namespace Hohnor
{
    class SocketAddrPair;

    class Socket : public FdGuard
    {
    public:
        typedef int SocketFd;
        //Initilize from existing fd
        Socket(SocketFd socketFd) : FdGuard(socketFd) {}
        //Initilize with paramters, in case failed, !!!abort the program
        Socket(int family, int type, int protocol) : FdGuard(SocketFuncs::socket(family, type, protocol)) {}

        //Get Tcp infomation. In case failed return !nullptr
        std::shared_ptr<struct tcp_info> getTCPInfo() const;
        //Get Tcp information string. In case failed return empty string
        std::string getTCPInfoStr() const;

        //Bind the address. In case failed, !!!abort the program
        void bindAddress(const InetAddress &localaddr) { SocketFuncs::bind(fd(), localaddr.getSockAddr()); }

        //Bind the address. In case failed, !!!abort the program
        void bindAddress(uint16_t port, bool loopbackOnly = false, bool ipv6 = false);

        //listen to the current socket. In case failed, !!!abort the program
        void listen() { SocketFuncs::listen(fd()); }

        //Accept a connection, gets a shared ptr to pair of connected socket and connected address
        //In case connection error, !nullptr will be returned
        SocketAddrPair accept();

        //Shutdown writing of the socket
        void shutdownWrite() { SocketFuncs::shutdownWrite(fd()); }

        // Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        void setTCPNoDelay(bool on);

        // Enable/disable SO_REUSEADDR
        void setReuseAddr(bool on);

        // Enable/disable SO_REUSEPORT
        void setReusePort(bool on);

        // Enable/disable SO_KEEPALIVE
        void setKeepAlive(bool on);

        //For clent connect
        void connect(const InetAddress &addr) { SocketFuncs::connect(fd(), addr.getSockAddr()); }

        ~Socket() = default;
    };

    /**
     * The (socketfd<-->address) pair that a server would get after accepting a TCP connection from client
     */
    class SocketAddrPair : Copyable
    {
    private:
        std::pair<Socket::SocketFd, std::shared_ptr<InetAddress>> pair_;

    public:
        friend class Socket;
        SocketAddrPair() : pair_(-1, new InetAddress) {}
        Socket::SocketFd &fd() { return std::get<0>(pair_); }
        InetAddress &addr() { return *std::get<1>(pair_); }
        std::shared_ptr<InetAddress> addrPtr() { return pair_.second; }
    };
} // namespace Hohnor
