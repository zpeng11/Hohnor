/**
 * Class wrapper for sockets, builds on SocketWrap.h
 */
#pragma once
#include "Types.h"
#include "SocketWrap.h"
#include "InetAddress.h"
#include <netinet/tcp.h>
#include <memory>
#include <map>

namespace Hohnor
{
    class Socket
    {
        typedef int SocketFd;

    private:
        /* data */
        SocketFd socketFd_;

    public:
        //Initilize from other fd
        Socket(SocketFd socketFd) : socketFd_(socketFd) {}
        //Initilize with paramters
        Socket(int family, int type, int protocol) : socketFd_(SocketFuncs::socket(family, type, protocol)) {}
        //Create a TCP nonblocking socket which can beused for epoll
        static Socket TCPNonblocking(sa_family_t family) { return SocketFuncs::nonBlockingSocket(family); }
        //get fd inside
        int fd() { return socketFd_; }
        //Get Tcp infomation, nullptr if failed
        std::shared_ptr<struct tcp_info> getTCPInfo() const;
        //Get Tcp information string, nullptr if failed
        std::shared_ptr<std::string> getTCPInfoStr() const;
        //Bind the address, if failed will abort the program
        void bindAddress(const InetAddress &localaddr) { SocketFuncs::bind(socketFd_, localaddr.getSockAddr()); }

        void listen() { SocketFuncs::listen(socketFd_); }

        std::shared_ptr<std::pair<SocketFd, InetAddress>> accept();

        ~Socket() { SocketFuncs::close(socketFd_); }
    };
} // namespace Hohnor
