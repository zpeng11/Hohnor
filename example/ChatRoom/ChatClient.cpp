#include <stdio.h>
#include <Socket.h>
#include <Logging.h>
#include <sys/epoll.h>
#include "Epoll.h"
#include "FdUtils.h"
#define SERVER_PORT 9342

using namespace std;
using namespace Hohnor;
int main(int argc, char *argv[])
{
    StringPiece user = argc < 3 ? "Guess" : argv[2];
    StringPiece server = argc < 2 ? "localhost" : argv[1];
    //Resolve input server address
    auto addres = InetAddress::resolve(server, std::to_string(SERVER_PORT));
    CHECK_NE(addres.size(), 0);
    //Connect to the server if found
    Socket socket(AF_INET, SOCK_STREAM, 0);
    socket.connect(addres[0]);
    //Epoll
    Hohnor::Epoll epoll;

    //add socket to epoll
    epoll.add(socket.fd(), EPOLLIN | EPOLLHUP);
    //Add stdin to epoll
    epoll.add(STDIN_FILENO, EPOLLIN);
    //prepare pipe for zero-copy IO splice
    int pipefd[2];
    CHECK_NE(pipe(pipefd), -1);
    FdGuard pipe0(pipefd[0]);
    FdGuard pipe1(pipefd[1]);

    while (1)
    {
        auto res = epoll.wait();
        while (res.hasNext())
        {
            auto &event = res.next();
            if (event.data.fd == socket.fd() && event.events & EPOLLIN) //recieved from the server
            {
                char buffer[BUFSIZ] = {0};
                read(socket.fd(), buffer, BUFSIZ);
                cout << buffer << endl;
            }
            else if (event.data.fd == socket.fd() && event.events & EPOLLHUP)
            {
                LOG_WARN << "Server ends";
                break;
            }
            else if (event.data.fd == STDIN_FILENO) //key board input
            {
                CHECK_NE(splice(STDIN_FILENO, NULL, pipefd[0], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MORE), -1);
                CHECK_NE(splice(pipefd[1], NULL, socket.fd(), NULL, 32768, SPLICE_F_MORE | SPLICE_F_MORE), -1);
            }
            else
            {
                LOG_INFO << "Unexpected epoll result";
            }
        }
    }//While loop
}