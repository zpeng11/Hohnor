#include "Socket.h"
#include <iostream>
using namespace std;
using namespace Hohnor;

int main()
{
    Socket s(AF_INET,SOCK_STREAM, 0);
    InetAddress ina(8888);
    s.connect(ina);
    char buf[1024];
    cout<<read(s.fd(),buf,1024)<<endl;;
    cout<<buf<<endl;
}