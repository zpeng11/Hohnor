/**
 * Socket for TCP, inherit from Hohnor::Socket
 */
#pragma once
#include "hohnor/net/Socket.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/net/TCPConnection.h"
#include <memory>

namespace Hohnor
{
    class EventLoop;
    typedef std::shared_ptr<EventLoop> EventLoopPtr;
    class IOHandler;
    typedef std::shared_ptr<IOHandler> IOHandlerPtr;
    class TCPAcceptor;
    typedef std::shared_ptr<TCPAcceptor> TCPAcceptorPtr;
    /**
     * TCP Listen Socket, used for server
     * It is a wrapper of ListenSocket, which is a Socket with listen(2) and accept(2) functions
     */
    class TCPAcceptor : public ListenSocket, public std::enable_shared_from_this<TCPAcceptor>
    {
    public:
        //Accept callback should take care if the handler is nullptr
        typedef std::function<void (TCPConnectionPtr)> AcceptCallback;
        
        // Static factory method to create shared_ptr instances
        static TCPAcceptorPtr create(EventLoopPtr loop, int options = SOCK_STREAM, bool ipv6 = false)
        {
            return TCPAcceptorPtr(new TCPAcceptor(loop, options, ipv6));
        }

        TCPAcceptor() = delete;

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
        
    protected:
        explicit TCPAcceptor(EventLoopPtr loop, int options = SOCK_STREAM, bool ipv6 = false)
            : ListenSocket(loop, ipv6 ? AF_INET6 : AF_INET, options | SOCK_STREAM, 0) {}
            
    private:
        //Actively accept a connection, return IOHandler
        IOHandlerPtr accept();

        //Hide setCallbacks from Socket into private, we don't need them
        using Socket::setReadCallback;
        using Socket::setWriteCallback;
        using Socket::setCloseCallback;
        using Socket::setErrorCallback;
        using Socket::enable;
        using Socket::isEnabled;
    };
} // namespace Hohnor
