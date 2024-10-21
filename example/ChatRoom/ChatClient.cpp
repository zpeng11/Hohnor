#include <stdio.h>
#include "hohnor/net/Socket.h"
#include "hohnor/log/Logging.h"
#include <sys/epoll.h>
#include <sys/wait.h>
#include "hohnor/io/Epoll.h"
#include "hohnor/io/FdUtils.h"
#include "hohnor/io/SignalUtils.h"
#define SERVER_PORT 9342

using namespace std;
using namespace Hohnor;
int main(int argc, char *argv[])
{

    StringPiece user = argc < 3 ? "Guess" : argv[2];
    StringPiece server = argc < 2 ? "localhost" : argv[1];
    //Resolve input server address
    auto addres = InetAddress::resolve(server, std::to_string(SERVER_PORT));
    HCHECK_NE(addres.size(), 0);
    LOG_INFO << addres[0].toIpPort();
    //Connect to the server if found
    Socket socket(AF_INET, SOCK_STREAM, 0);
    socket.connect(addres[0]);
    //Epoll
    Hohnor::Epoll epoll;

    bool stopServer = false;
    SignalUtils::handleSignal(SIGINT);

    //add socket to epoll
    epoll.add(socket.fd(), EPOLLIN | EPOLLHUP);
    //Add stdin to epoll
    epoll.add(STDIN_FILENO, EPOLLIN);
    //Add signalHandler to epoll
    epoll.add(SignalUtils::readEndFd(), EPOLLIN);

    //prepare pipe for zero-copy IO splice
    int pipefd[2];
    HCHECK_NE(pipe(pipefd), -1);

    char buf[BUFSIZ] = {0};
    while (!stopServer)
    {
        auto res = epoll.wait();
        while (res.hasNext())
        {
            auto &event = res.next();
            if (event.data.fd == socket.fd() && event.events & EPOLLIN) //recieved from the server
            {
                memZero(buf, BUFSIZ);
                int ret = read(socket.fd(), buf, BUFSIZ);
                if (ret < 0 && errno != EAGAIN)
                {
                    LOG_SYSERR << "Read from client error";
                }
                else if (ret == 0 && errno != EINTR)
                {
                    epoll.remove(event.data.fd);
                    LOG_INFO << "Server logout";
                    stopServer = true;
                }
            }
            else if (event.data.fd == socket.fd() && event.events & EPOLLHUP)
            {
                LOG_WARN << "Server ends";
                stopServer = true;
            }
            else if (event.data.fd == SignalUtils::readEndFd() && event.events == EPOLLIN)
            {
                auto iter = SignalUtils::receive();
                while (iter.hasNext())
                {
                    char sig = iter.next();
                    LOG_INFO<<"Got signal:"<<(int)sig;
                }
            }
            else if (event.data.fd == STDIN_FILENO) //key board input
            {
                int ret = splice(STDIN_FILENO, NULL, pipefd[1], NULL, BUFSIZ, SPLICE_F_MORE | SPLICE_F_MORE);
                if (ret < 0)
                    LOG_SYSERR << "Splice error";
                ret = splice(pipefd[0], NULL, socket.fd(), NULL, BUFSIZ, SPLICE_F_MORE | SPLICE_F_MORE);
                if (ret < 0)
                    LOG_SYSERR << "Splice error";
            }

            else
            {
                LOG_INFO << "Unexpected epoll result";
            }
        }
    } //While loop
}