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
#include "ProcessInfo.h"
#include "TCPSocket.h"
#include "BinaryHeap.h"
using namespace std;

using namespace Hohnor;

int main(int argc, char *argv[])
{
    // TCPListenSocket s;
    // s.bindAddress(8888);
    // s.listen();
    // auto ptr = s.accept();
    // cout<<ptr.addr().toIpPort()<<endl;
    // char str[] = "Hellow world";
    // SocketFuncs::write(ptr.fd(), str, strlen(str));
    BinaryHeap<int> heap([](int &lhs, int &rhs)
                         { return lhs < rhs; });
    for(int i = 10; i>-1;i--)
    {
        heap.insert(i);
    }
    int a = 1;
}