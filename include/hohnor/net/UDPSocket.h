/**
 * Socket for UDP, inherit from Hohnor::Socket
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
     * UDP Socket, used for both client and server
     * It is a wrapper of Socket, which provides UDP-specific functionality
     * Unlike TCP, UDP doesn't need separate listen/accept mechanisms
     */
    class UDPSocket : public Socket
    {
    public:
        explicit UDPSocket(std::shared_ptr<IOHandler> socketHandler) : Socket(socketHandler) {}
        explicit UDPSocket(std::shared_ptr<EventLoop> loop, int options = SOCK_DGRAM, bool ipv6 = false)
            : Socket(loop, ipv6 ? AF_INET6 : AF_INET, options | SOCK_DGRAM, 0) {}

        // Send data to a specific address (for client or server)
        ssize_t sendTo(const void* data, size_t len, const InetAddress& addr);
        
        // Receive data from any address (returns sender address)
        ssize_t recvFrom(void* buffer, size_t len, InetAddress& fromAddr);
        
        // Send data using connected UDP socket
        ssize_t send(const void* data, size_t len);
        
        // Receive data using connected UDP socket
        ssize_t recv(void* buffer, size_t len);

        // Enable/disable SO_BROADCAST for broadcast packets
        void setBroadcast(bool on);
        
        // Set multicast TTL
        void setMulticastTTL(int ttl);
        
        // Join multicast group
        void joinMulticastGroup(const InetAddress& groupAddr, const InetAddress& interfaceAddr = InetAddress());
        
        // Leave multicast group
        void leaveMulticastGroup(const InetAddress& groupAddr, const InetAddress& interfaceAddr = InetAddress());
        
        // Enable/disable multicast loopback
        void setMulticastLoopback(bool on);
        
        // Set receive buffer size
        void setRecvBufferSize(int size);
        
        // Set send buffer size
        void setSendBufferSize(int size);
        
        // Get receive buffer size
        int getRecvBufferSize() const;
        
        // Get send buffer size
        int getSendBufferSize() const;
    };

    /**
     * UDP Listen Socket, used for server applications
     * Inherits from UDPSocket but provides server-specific interface
     */
    class UDPListenSocket : public UDPSocket
    {
    private:
        // Hide connect function as it's not typically used for UDP servers
        int connect(const InetAddress &addr, bool blockin);
        
    public:
        explicit UDPListenSocket(std::shared_ptr<IOHandler> socketHandler) : UDPSocket(socketHandler) {}
        explicit UDPListenSocket(std::shared_ptr<EventLoop> loop, int options = SOCK_DGRAM, bool ipv6 = false)
            : UDPSocket(loop, options, ipv6) {}

        // Bind the address. In case failed, !!!abort the program
        void bindAddress(const InetAddress &localaddr) { SocketFuncs::bind(fd(), localaddr.getSockAddr()); }

        // Bind the address. In case failed, !!!abort the program
        void bindAddress(uint16_t port, bool loopbackOnly = false, bool ipv6 = false);

        // Enable/disable SO_REUSEADDR
        void setReuseAddr(bool on);

        // Enable/disable SO_REUSEPORT
        void setReusePort(bool on);

        // Set callback for incoming data
        void setDataCallback(ReadCallback cb) { setReadCallback(std::move(cb)); }
    };
} // namespace Hohnor