#include <vector>
#include "Types.h"
#include "Socket.h"
#include "Epoll.h"
#include "StringPiece.h"
#include "FdUtils.h"
#include "SocketWrap.h"
#include "TCPSocket.h"
#define SERVER_PORT 9342

using namespace Hohnor;
int main()
{
    TCPListenSocket listenSocket;
    listenSocket.bindAddress(SERVER_PORT);
    listenSocket.listen();
    Epoll epoll;
    epoll.add(listenSocket.fd(), EPOLLIN | EPOLLERR );
    while (1)
    {
        auto res = epoll.wait();
        while(res.hasNext())
        {
            auto& event = res.next();
            if(event.data.fd == listenSocket.fd() && (event.events & EPOLLIN))//Client request to connect
            {
                auto sockAddrPair = listenSocket.accept();
                FdUtils::setNonBlocking(sockAddrPair.fd());
                epoll.add(sockAddrPair.fd(), EPOLLIN| EPOLLRDHUP | EPOLLERR);
                LOG_INFO<<"Get a client connected";
            }
            else if(event.events & EPOLLERR)
            {
                int error = SocketFuncs::getSocketError(event.data.fd);
                LOG_ERROR<<Hohnor::strerror_tl(error);
            }
        }
    }
    
}