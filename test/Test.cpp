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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


using namespace std;

using namespace Hohnor;

int main(int argc, char *argv[])
{
    CHECK(argc>1);
    struct sockaddr_in6 sai6;
    auto res = InetAddress::resolve(argv[1]);
    for(auto ina :res)
    {
        LOG_INFO<<ina.toIpPort();
    }
    CHECK(false);
}