/**
 * Socket for TCP, inherit from Hohnor::Socket
 */
#pragma once
#include "hohnor/net/Socket.h"
#include "hohnor/common/Callbacks.h"
#include <memory>

namespace Hohnor
{
    class EventLoop;
    class IOHandler;
    /**
     * TCP Listen Socket, used for server
     * It is a wrapper of ListenSocket, which is a Socket with listen(2) and accept(2) functions
     */
    class TCPAcceptor : public ListenSocket, public std::enable_shared_from_this<TCPAcceptor>
    {
    public:
        //Accept callback should take care if the handler is nullptr
        typedef std::function<void (std::shared_ptr<IOHandler>, InetAddress)> AcceptCallback;
        explicit TCPAcceptor(std::shared_ptr<EventLoop> loop, int options = SOCK_STREAM, bool ipv6 = false)
            : ListenSocket(loop, ipv6 ? AF_INET6 : AF_INET, options | SOCK_STREAM, 0) {}

        ~TCPAcceptor() = default;

        //Get Tcp infomation. In case failed return !nullptr
        struct tcp_info getTCPInfo() const;
        //Get Tcp information string. In case failed return empty string
        std::string getTCPInfoStr() const;

        void listen()
        {
            SocketFuncs::listen(fd());
            Socket::enable();
        }

        bool isListening(){
            return isEnabled();
        }

        using Socket::disable;

        void setAcceptCallback(AcceptCallback cb);

        //Shutdown writing of the socket
        void shutdownWrite() { SocketFuncs::shutdownWrite(fd()); }

        // Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        void setTCPNoDelay(bool on);

        // Enable/disable SO_KEEPALIVE
        void setKeepAlive(bool on);
    private:
        //Actively accept a connection, return a pair of IOHandler and InetAddress, can be
        //used for in a socket read callback
        std::pair<std::shared_ptr<IOHandler>, InetAddress> accept();

        //Hide setCallbacks from Socket into private, we don't need them
        using Socket::setReadCallback;
        using Socket::setWriteCallback;
        using Socket::setCloseCallback;
        using Socket::setErrorCallback;
        using Socket::enable;
        using Socket::isEnabled;
    };
} // namespace Hohnor
