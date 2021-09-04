#include <stdio.h>
#include <Socket.h>
#include <Logging.h>
#include <sys/epoll.h>

#define SERVER_PORT 9342
#define MAX_EVENTS 10

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
    struct epoll_event ev = {0};
    struct epoll_event events[MAX_EVENTS] = {0};
    int epollfd = epoll_create1(0);
    CHECK_NE(epollfd, -1);
    //add socket to epoll
    ev.events = EPOLLIN | EPOLLHUP;
    ev.data.fd = socket.fd();
    CHECK_NE(epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev), -1);
    //Add stdin to epoll
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    CHECK_NE(epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev), -1);
    //prepare pipe for zero-copy IO splice
    int pipefd[2];
    CHECK_NE(pipe(pipefd), -1);
    while (1)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        CHECK_NE(nfds, -1);
        for (int i = 0; i < nfds; i++)
        {
            struct epoll_event &curEvent = events[i];
            if (curEvent.data.fd == socket.fd() && curEvent.events & EPOLLIN) //Server sent
            {
                char buffer[BUFSIZ] = {0};
                read(socket.fd(), buffer, BUFSIZ);
                cout << buffer << endl;
            }
            else if (curEvent.data.fd == socket.fd() && curEvent.events & EPOLLHUP) //Server closed
            {
                LOG_WARN << "Server ends";
                break;
            }
            else if (events[i].data.fd == STDIN_FILENO) //input
            {
                CHECK_NE(splice(STDIN_FILENO,NULL,pipefd[0], NULL,32768, SPLICE_F_MORE | SPLICE_F_MORE), -1);
                CHECK_NE(splice(pipefd[1],NULL,socket.fd(), NULL,32768, SPLICE_F_MORE | SPLICE_F_MORE), -1);
            }
            else
            {
                LOG_INFO << "Unexpected epoll result";
            }
        }
    }
    close(epollfd);
}