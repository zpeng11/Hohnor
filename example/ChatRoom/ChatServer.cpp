#include <vector>
#include "Types.h"
#include "Socket.h"
#include "Epoll.h"
#include "StringPiece.h"
#include "FdUtils.h"
#include "SocketWrap.h"
#include "TCPSocket.h"
#include <map>
#include <algorithm>
#define SERVER_PORT 9342

struct ClientData
{
    char buf[BUFSIZ];
    char *toWrite;
};

using namespace Hohnor;
int main()
{
    TCPListenSocket listenSocket;
    listenSocket.bindAddress(SERVER_PORT);
    listenSocket.listen();
    Epoll epoll;
    epoll.add(listenSocket.fd(), EPOLLIN | EPOLLERR);
    std::map<int, ClientData> users;
    while (1)
    {
        auto res = epoll.wait();
        while (res.hasNext())
        {
            auto &event = res.next();
            if (event.data.fd == listenSocket.fd() && (event.events & EPOLLIN)) //Client request to connect
            {
                auto sockAddrPair = listenSocket.accept();
                FdUtils::setNonBlocking(sockAddrPair.fd());
                epoll.add(sockAddrPair.fd(), EPOLLIN | EPOLLRDHUP | EPOLLERR);
                users.insert(std::pair<int, ClientData>(sockAddrPair.fd(), std::move(ClientData())));
                LOG_INFO << "Get a client connected, user size:" << users.size();
            }
            else if (event.events & EPOLLERR)
            {
                int error = SocketFuncs::getSocketError(event.data.fd);
                LOG_ERROR << Hohnor::strerror_tl(error);
            }
            else if (event.events & EPOLLRDHUP)
            {
                epoll.remove(event.data.fd);
                FdUtils::close(event.data.fd);
                users.erase(event.data.fd);
                LOG_INFO << "User logout, rest users:" << users.size();
            }
            else if (event.events & EPOLLIN)
            {
                memZero(users[event.data.fd].buf, BUFSIZ);
                int ret = read(event.data.fd, users[event.data.fd].buf, BUFSIZ);
                if (ret < 0 && errno != EAGAIN)
                {
                    LOG_SYSERR << "Read from client error";
                }
                else if (ret == 0 && errno != EINTR)
                {
                    LOG_INFO << "User unexpected disconnect";
                    epoll.remove(event.data.fd);
                    FdUtils::close(event.data.fd);
                    users.erase(event.data.fd);
                    LOG_INFO << "User logout, rest users:" << users.size();
                    break;
                }
                else
                {
                    LOG_INFO << "Get message: " << users[event.data.fd].buf;
                    for (auto &pair : users)
                    {
                        if (pair.first != event.data.fd)
                        {
                            epoll.modify(pair.first, EPOLLOUT | EPOLLERR);
                            pair.second.toWrite = users[event.data.fd].buf;
                        }
                    }
                }
            }
            else if (event.events & EPOLLOUT)
            {
                int connfd = event.data.fd;
                if (!users[connfd].toWrite)
                {
                    continue;
                }
                int ret = send(connfd, users[connfd].toWrite, BUFSIZ, 0);
                users[connfd].toWrite = NULL;
                epoll.modify(connfd, EPOLLIN | EPOLLERR);
            }
            else
            {
                LOG_ERROR << "Unexpected epoll result";
            }
        }
    }
    for (auto &pair : users)
    {
        FdUtils::close(pair.first);
    }
}