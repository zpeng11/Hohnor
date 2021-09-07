/**
 * Socket for TCP, inherit from Hohnor::Socket
 */
#pragma once
#include "Socket.h"

namespace Hohnor
{
    class SocketAddrPair;
    class TCPListenSocket : public ListenSocket
    {
    public:
        explicit TCPListenSocket(Socket::SocketFd fd) : ListenSocket(fd) {}
        explicit TCPListenSocket(int options = SOCK_STREAM, bool ipv6 = false)
            : ListenSocket(ipv6 ? AF_INET6 : AF_INET, options | SOCK_STREAM, 0) {}

        //Get Tcp infomation. In case failed return !nullptr
        std::shared_ptr<struct tcp_info> getTCPInfo() const;
        //Get Tcp information string. In case failed return empty string
        std::string getTCPInfoStr() const;

        //Accept a connection, gets a shared ptr to pair of connected socket and connected address
        //In case connection error, !nullptr will be returned
        SocketAddrPair accept();
        //accept a connection, return the connected fd, and fill InetAddress * returnAddr,
        //This is faster than returning SocketAddrPair by avoiding copying
        //returnAddr can be NULL if you do not care about peer address
        int accept(InetAddress * returnAddr);

        //Shutdown writing of the socket
        void shutdownWrite() { SocketFuncs::shutdownWrite(fd()); }

        // Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        void setTCPNoDelay(bool on);

        // Enable/disable SO_KEEPALIVE
        void setKeepAlive(bool on);
    };

    /**
     * The (socketfd<-->address) pair that a server would get after accepting a TCP connection from client
     */
    class SocketAddrPair : public Copyable
    {
    private:
        std::pair<Socket::SocketFd, InetAddress> pair_;

    public:
        friend class TCPListenSocket;
        SocketAddrPair() : pair_() {}
        Socket::SocketFd &fd() { return std::get<0>(pair_); }
        InetAddress &addr() { return std::get<1>(pair_); }
        InetAddress* addrPtr() { return &pair_.second; }
    };
} // namespace Hohnor
