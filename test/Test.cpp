#include <iostream>
#include <stdio.h>
#include "Thread.h"
#include "Timestamp.h"
#include "Date.h"
#include "Logging.h"
#include "FileUtils.h"
#include "ProcessInfo.h"
#include "ThreadPool.h"
#include <memory>
#include "LogFile.h"
#include "LogFile.h"
#include "AsyncLogging.h"
#include "SocketWrap.h"
#include "InetAddress.h"
#include "Socket.h"

using namespace std;

using namespace Hohnor;


int main(int argc, char *argv[])
{
    Socket s(AF_INET,SOCK_STREAM,0);
    s.bindAddress(8888);
    s.listen();
    auto ptr = s.accept();
    cout<<ptr->addr().toIpPort()<<endl;
    string str = "Helloworld";
    write(ptr->socket().fd(), str.c_str(), str.length()+1);
    cout<<ptr->socket().getTCPInfoStr()<<endl;
}