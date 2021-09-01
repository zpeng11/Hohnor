/**
 * Class wrapper for sockets, builds on SocketWrap.h
 */
#pragma once
#include "Types.h"
#include "SocketWrap.h"
#include "InetAddress.h"
#include "NonCopyable.h"
#include <netinet/tcp.h>
#include <memory>
#include <map>

namespace Hohnor
{
    class Socket : NonCopyable
    {
        typedef int SocketFd;

    private:
        /* data */
        SocketFd socketFd_;

    public:
        //Initilize from other fd
        Socket(SocketFd socketFd) : socketFd_(socketFd) {}
        //Initilize with paramters, in case failed, !!!abort the program
        Socket(int family, int type, int protocol) : socketFd_(SocketFuncs::socket(family, type, protocol)) {}

        //get fd inside
        int fd() { return socketFd_; }

        //Get Tcp infomation. In case failed return !nullptr
        std::shared_ptr<struct tcp_info> getTCPInfo() const;
        //Get Tcp information string. In case failed return !nullptr
        std::shared_ptr<std::string> getTCPInfoStr() const;

        //Bind the address. In case failed, !!!abort the program
        void bindAddress(const InetAddress &localaddr) { SocketFuncs::bind(socketFd_, localaddr.getSockAddr()); }

        //Bind the address. In case failed, !!!abort the program
        void bindAddress(uint16_t port, bool loopbackOnly = false, bool ipv6 = false);

        //listen to the current socket. In case failed, !!!abort the program
        void listen() { SocketFuncs::listen(socketFd_); }

        //Accept a connection, gets a shared ptr to pair of connected socket and connected address
        //In case connection error, !nullptr will be returned
        std::shared_ptr<std::pair<SocketFd, InetAddress>> accept();

        //Shutdown writing of the socket
        void shutdownWrite() { SocketFuncs::shutdownWrite(socketFd_); }

        // Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        void setTCPNoDelay(bool on);

        // Enable/disable SO_REUSEADDR
        void setReuseAddr(bool on);

        // Enable/disable SO_REUSEPORT
        void setReusePort(bool on);

        // Enable/disable SO_KEEPALIVE
        void setKeepAlive(bool on);

        ~Socket() { SocketFuncs::close(socketFd_); }
    };
} // namespace Hohnor
