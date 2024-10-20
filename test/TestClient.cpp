#include "hohnor/net/Socket.h"
#include <iostream>
using namespace std;
using namespace Hohnor;

int main()
{
    Socket s(AF_INET,SOCK_STREAM, 0);
    InetAddress ina(9911);
    s.connect(ina);
    char buf[1024] = "Hello world";
    ::write(s.fd(),buf,1024);
}