/**
 * Wrappers for linux socket c api that is error free(error are mostly handled)
 */
#pragma once
#include "StringPiece.h"
#include "Types.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/uio.h> // readv
#include <netinet/in.h>
#include <endian.h>

namespace Hohnor
{
    namespace SocketFuncs
    {
        typedef int SocketFd;
        //wrapped socket(2) api
        SocketFd socket(int __family, int __type, int __protocol);
        //Convient nonblocking socket creation
        //Since we are making Reactive model server, most likely using this one
        //family could be AF_INET, AF_UNIX, or AF_INET6
        SocketFd nonBlockingSocket(sa_family_t family);

        //wrapped bind api, we only use sockaddr ptr, use type cast to convert from other address type
        void bind(SocketFd socketfd, const struct sockaddr *addr);
        //wrapped listen(2) api
        void listen(SocketFd socketfd);
        //Wrapped accept(2) api
        SocketFd accept(int sockfd, struct sockaddr_in6 *addr);

        //Wrapped close(3) api
        void close(SocketFd socketfd);
        //Wrapped shutdown(3) writing
        void shutdownWrite(SocketFd socketfd);

        //Wrapper of inet_ntop(3), that converts from a sockaddr to ip&port string
        string toIpPort(const struct sockaddr *addr);
        //Wrapper of inet_ntop(3), that converts from a sockaddr to ip string
        string toIp(const struct sockaddr *addr);

        //Wrapper for inet_pton(3), that converts ip&port into sockaddr
        void fromIpPort(StringPiece ipStr, uint16_t port,
                        struct sockaddr_in *addr);
        //Wrapper for inet_pton(3), that converts ip&port into sockaddr
        void fromIpPort(StringPiece ipStr, uint16_t port,
                        struct sockaddr_in6 *addr);

        //get errono from socket
        int getSocketError(int sockfd);

        //wrapped connect(2) api, used only for clients
        void connect(SocketFd socketfd, const struct sockaddr *addr);

        //Wrapper for getsockname(2)
        struct sockaddr_in6 getLocalAddr(int sockfd);
        //Wrapper for getpeername(2)
        struct sockaddr_in6 getPeerAddr(int sockfd);
        bool isSelfConnect(int sockfd);

        //Direct use for read(2)
        constexpr auto read = ::read;
        //Direct use for write(2)
        constexpr auto write = ::write;
        //Direct use for readv(2)
        constexpr auto readv = ::readv;
        //Direct use for writev(2)
        constexpr auto writev = ::writev;

        //Static cast to struct sockaddr
        inline const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr)
        {
            return static_cast<const struct sockaddr *>(implicit_cast<const void *>(addr));
        }

        //Static cast to struct sockaddr
        inline struct sockaddr *sockaddr_cast(struct sockaddr_in6 *addr)
        {
            return static_cast<struct sockaddr *>(implicit_cast<void *>(addr));
        }

        //Static cast to struct sockaddr
        inline const struct sockaddr *sockaddr_cast(const struct sockaddr_in *addr)
        {
            return static_cast<const struct sockaddr *>(implicit_cast<const void *>(addr));
        }

        //Static cast to struct sockaddr_in
        inline const struct sockaddr_in *sockaddr_in_cast(const struct sockaddr *addr)
        {
            return static_cast<const struct sockaddr_in *>(implicit_cast<const void *>(addr));
        }

        //Static cast to struct sockaddr_in6
        inline const struct sockaddr_in6 *sockaddr_in6_cast(const struct sockaddr *addr)
        {
            return static_cast<const struct sockaddr_in6 *>(implicit_cast<const void *>(addr));
        }

        //Endian convertion
        inline uint64_t hostToNetwork64(uint64_t host64)
        {
            return htobe64(host64);
        }

        //Endian convertion
        inline uint32_t hostToNetwork32(uint32_t host32)
        {
            return htobe32(host32);
        }

        //Endian convertion
        inline uint16_t hostToNetwork16(uint16_t host16)
        {
            return htobe16(host16);
        }

        //Endian convertion
        inline uint64_t networkToHost64(uint64_t net64)
        {
            return be64toh(net64);
        }

        //Endian convertion
        inline uint32_t networkToHost32(uint32_t net32)
        {
            return be32toh(net32);
        }

        //Endian convertion
        inline uint16_t networkToHost16(uint16_t net16)
        {
            return be16toh(net16);
        }
    } // namespace socketFuncs

} // namespace Hohnor
