/**
 * Wrappers for linux socket c api that is error free(error are mostly handled)
 */
#pragma once
#include <errno.h>
#include <fcntl.h>
#include <stdio.h> // snprintf
#include <sys/socket.h>
#include <sys/uio.h> // readv
#include <unistd.h>

namespace Hohnor
{
    namespace Socket
    {
        typedef int SocketFd;
        //wrapped socket(2) api
        SocketFd socket(int __family, int __type, int __protocol);
        //Convient nonblocking socket creation
        //Since we are making Reactive model server, most likely using this one
        SocketFd nonBlockingSocket(sa_family_t family);
        //wrapped bind api, we only use sockaddr ptr, use type cast to convert from other address type
        void bind(SocketFd socketfd, const struct sockaddr *addr);
        //wrapped listen(2) api
        void listen(SocketFd socketfd);
        //Wrapped accept(2) api
        SocketFd accept();
        //Wrapped close(3) api
        void close(SocketFd socketfd);
        //Wrapped shutdown(3) writing
        void shutdownWrite(SocketFd socketfd);

        void toIpPort(char *buf, size_t size,
                      const struct sockaddr *addr);
        void toIp(char *buf, size_t size,
                  const struct sockaddr *addr);

        void fromIpPort(const char *ip, uint16_t port,
                        struct sockaddr_in *addr);
        void fromIpPort(const char *ip, uint16_t port,
                        struct sockaddr_in6 *addr);

        //wrapped connect(2) api, used only for clients
        void connect(SocketFd socketfd, const struct sockaddr *addr);
        /**
         * For read(2), write(2), readv(2), writev(2), we should not do more wrapping in this level
         */

    } // namespace socket

} // namespace Hohnor
