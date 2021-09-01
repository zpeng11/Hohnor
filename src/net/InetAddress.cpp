#include "InetAddress.h"
#include "Logging.h"
#include "Types.h"
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

using namespace Hohnor;

/**
 * Ensure that memory layout are standrad layout as we expected above;
 */
static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

InetAddress::InetAddress(uint16_t portArg, bool loopbackOnly, bool ipv6)
{
    static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
    static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
    if (ipv6)
    {
        memZero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = SocketFuncs::hostToNetwork16(portArg);
    }
    else
    {
        memZero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = SocketFuncs::hostToNetwork32(ip);
        addr_.sin_port = SocketFuncs::hostToNetwork16(portArg);
    }
}

InetAddress::InetAddress(StringPiece ip, uint16_t portArg, bool ipv6)
{
    if (ipv6 || strchr(ip.data(), ':')) //xxxx:xxxx... format ipv6 address
    {
        memZero(&addr6_, sizeof addr6_);
        SocketFuncs::fromIpPort(ip, portArg, &addr6_);
    }
    else
    {
        memZero(&addr_, sizeof addr_);
        SocketFuncs::fromIpPort(ip, portArg, &addr_);
    }
}

string InetAddress::toIpPort() const
{
    return std::move(SocketFuncs::toIpPort(getSockAddr()));
}

string InetAddress::toIp() const
{
    return std::move(SocketFuncs::toIp(getSockAddr()));
}

uint16_t InetAddress::port() const
{
    return SocketFuncs::networkToHost16(portNetEndian());
}

uint32_t InetAddress::ipv4NetEndian() const
{
    CHECK_EQ(this->family(), AF_INET);
    return addr_.sin_addr.s_addr;
}

using namespace std;
#include "LogStream.h"
bool InetAddress::resolve(StringPiece hostname, InetAddress *out)
{
    CHECK_NOTNULL(out);
    struct addrinfo hints; //provice hints for getaddrinfo(3)
    memZero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo *result;
    int ret = getaddrinfo(hostname.data(), "http", &hints, &result);
    if (ret != 0)
    {
        LOG_SYSERR << "InetAddress::resolve DNS service error, " << gai_strerror(ret);
        return false;
    }

    //Until now we get a struct addrinfo linked list head object pointed by @result on heap
    printf("%p\n", result);
    cout << result->ai_addrlen << endl;
    cout << result->ai_canonname << endl;
    cout << result->ai_family << endl;
    cout << "Debug" << endl;
    int sfd;
    struct addrinfo *rp;
    for (rp = result; rp != NULL; rp = rp->ai_next) //iterate the linked list addrinfo struct
    {
        cout << SocketFuncs::toIp(rp->ai_addr) << endl;
        sfd = SocketFuncs::socket(rp->ai_family, rp->ai_socktype,
                                  rp->ai_protocol);
        if (sfd == -1)
        {
            LOG_WARN << "InetAddress::resolve try for a result failed";
        }
        else
        {
            if (::connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
                break; /* Success */

            SocketFuncs::close(sfd);
        }
    }
    if (rp == NULL) //There is not one of the found address we can connect to
    {               /* No address succeeded */
        LOG_SYSERR << "InetAddress::resolve could not connect to any address found";
        freeaddrinfo(result); //clear the linked list in heap
        return false;
    }
    else //we found one address that can be connected to
    {
        SocketFuncs::close(sfd); //close the successful connection
        memZero(out, sizeof(sockaddr_in6));
        memcpy(out, rp->ai_addr, rp->ai_addrlen); //write result output
        freeaddrinfo(result);                     //clear the linked list in heap
        return true;
    }
}