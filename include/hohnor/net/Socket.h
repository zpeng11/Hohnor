/**
 * Class wrapper for sockets, builds on SocketWrap.h
 */
#pragma once
#include "hohnor/common/Types.h"
#include "hohnor/net/SocketWrap.h"
#include "hohnor/net/InetAddress.h"
#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/io/FdUtils.h"
#include <netinet/tcp.h>
#include <memory>

namespace Hohnor
{
    class IOHandler;
    class EventLoop;

    /*
    * General Socket class, used for both client and server(mostly for client), it takes care of the socket fd
    * and fd's lifecycle & ownership And provides some basic functions like connect, get local/peer address, etc. 
    */
    class Socket : public NonCopyable
    {
    private:
        std::shared_ptr<IOHandler> socketHandler_;
        std::shared_ptr<EventLoop> loop_;
    protected:
        void resetSocketHandler(std::shared_ptr<IOHandler> handler = nullptr);
        Socket(std::shared_ptr<IOHandler> handler, std::shared_ptr<EventLoop> loop){
            socketHandler_ = handler;
            loop_ = loop;
        }
        std::shared_ptr<IOHandler> getSocketHandler() const { return socketHandler_; }
    public:
        typedef int SocketFd;
        ~Socket();

        //Delete default constructor
        Socket() = delete;
        
        //Initilize with paramters, in case failed, !!!abort the program
        Socket(std::shared_ptr<EventLoop> loop, int family, int type, int protocol = 0);

        //For clent connect, you can set nonBlocking to true, so that it will not block the thread for waiting handshake, return 0 on success, -1 on error
        int connect(const InetAddress &addr);

        //Socket error
        int getSockError() { return SocketFuncs::getSocketError(fd()); }
        //Get socket error string
        string getSockErrorStr() { return strerror_tl(getSockError()); }

        InetAddress getLocalAddr() { return InetAddress(SocketFuncs::getLocalAddr(fd())) ; }
        InetAddress getPeerAddr() { return InetAddress(SocketFuncs::getPeerAddr(fd())) ; }
        bool isSelfConnect() { return SocketFuncs::isSelfConnect(fd()); }

        SocketFd fd() const;

        std::shared_ptr<EventLoop> loop();

        //Expose IOHandler's interface
        void setReadCallback(ReadCallback cb);
        void setWriteCallback(WriteCallback cb);
        void setCloseCallback(CloseCallback cb);
        void setErrorCallback(ErrorCallback cb);
        void enable();
        void disable();
        bool isEnabled();
    };

    /*
    * General Listen Socket class, used for server
    */
    class ListenSocket : public Socket
    {
    private:
        int connect(const InetAddress &addr);
        InetAddress getLocalAddr();
        InetAddress getPeerAddr();
        bool isSelfConnect();

    public:
        ListenSocket(std::shared_ptr<EventLoop> loop, int family, int type, int protocol = 0)
            : Socket(loop, family, type, protocol) {}

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
