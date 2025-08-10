#include "hohnor/net/TCPConnector.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/log/Logging.h"
#include "hohnor/time/Timestamp.h"
#include "hohnor/io/FdUtils.h"
#include <errno.h>
#include <algorithm>
#include <math.h>

using namespace Hohnor;

TCPConnector::TCPConnector(EventLoopPtr loop, const InetAddress& addr)
    : Socket(loop, AF_INET, SOCK_STREAM),
      serverAddr_(addr),
      constantDelay_(false),
      retryDelayMs_(DefaultRetryDelayMs),
      maxRetries_(0),
      currentRetries_(0),
      newConnectionCallback_(),
      retryCallback_(),
      failedCallback_(),
      state_(Disconnected)
{
    LOG_DEBUG << "TCPConnector created for " << serverAddr_.toIpPort();
}

void TCPConnector::setNewConnectionCallback(NewConnectionCallback cb)
{
    std::weak_ptr<TCPConnector> weakThis = shared_from_this();
    loop()->runInLoop([weakThis, cb]() {
        auto sharedThis = weakThis.lock();
        if (sharedThis) {
            sharedThis->newConnectionCallback_ = std::move(cb);
        } else {
            LOG_WARN << "TCPConnector is already destroyed, cannot set new connection callback";
        }
    });
}

void TCPConnector::setRetryConnectionCallback(RetryConnectionCallback cb)
{
    std::weak_ptr<TCPConnector> weakThis = shared_from_this();
    loop()->runInLoop([weakThis, cb]() {
        auto sharedThis = weakThis.lock();
        if (sharedThis) {
            sharedThis->retryCallback_ = std::move(cb);
        } else {
            LOG_WARN << "TCPConnector is already destroyed, cannot set retry connection callback";
        }
    });
}

void TCPConnector::setFailedConnectionCallback(FailedConnectionCallback cb)
{
    std::weak_ptr<TCPConnector> weakThis = shared_from_this();
    loop()->runInLoop([weakThis, cb]() {
        auto sharedThis = weakThis.lock();
        if (sharedThis) {
            sharedThis->failedCallback_ = std::move(cb);
        } else {
            LOG_WARN << "TCPConnector is already destroyed, cannot set failed connection callback";
        }
    });
}

void TCPConnector::start()
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis]() {
        if (sharedThis->state_ != Disconnected) {
            LOG_WARN << "TCPConnector restarting";
            sharedThis->stop();
        }
        
        sharedThis->currentRetries_ = 0;
        if(!sharedThis->constantDelay_) {
            sharedThis->retryDelayMs_ = TCPConnector::DefaultRetryDelayMs; 
        }
        
        LOG_DEBUG << "TCPConnector starting connection to " << sharedThis->serverAddr_.toIpPort()
                  << " with retryDelay=" << sharedThis->retryDelayMs_ << "ms, retries=" << sharedThis->maxRetries_;
        
        sharedThis->connect(sharedThis->serverAddr_);
    });
}

void TCPConnector::stop()
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis]() {
        if (sharedThis->state_ == Disconnected) {
            LOG_WARN << "TCPConnector::stop() called but already Disconnected";
            return;
        }

        if (sharedThis->state_ == Connected && sharedThis->getSocketHandler() == nullptr) {
            LOG_DEBUG << "TCPConnector in connected state, stopping succeeded connector to " << sharedThis->serverAddr_.toIpPort();
            LOG_DEBUG << "This will not affect the established connection.";
        }
        else
        {
            LOG_DEBUG << "TCPConnector in connecting state, stopping running connector to " << sharedThis->serverAddr_.toIpPort();
            // Reset the socket handler to clean up the connection
            sharedThis->disable();
            sharedThis->resetSocketHandler(nullptr);
        }

        sharedThis->state_ = Disconnected;
        // Create a new socket for potential future connections
        int newFd = SocketFuncs::socket(AF_INET, SOCK_STREAM, 0);

        auto newHandler = sharedThis->loop()->handleIO(newFd);
        sharedThis->resetSocketHandler(newHandler);
    });
}

int TCPConnector::connect(const InetAddress& addr)
{
    this->loop()->assertInLoopThread();
    setWriteCallback(nullptr);
    setErrorCallback(nullptr);
    
    HCHECK(state_ == Disconnected) << "TCPConnector state must be Disconnected";
    
    state_ = Connecting;
    FdUtils::setNonBlocking(fd(), true);
    LOG_DEBUG << "TCPConnector connecting to " << addr.toIpPort();
    int ret = Socket::connect(addr);
    int savedErrno = errno;
    
    switch (savedErrno) {
        case 0:
        case EISCONN:
            LOG_DEBUG << "TCPConnector connected immediately :)";
        case EINPROGRESS:
        case EINTR:
            // Connection in progress
            LOG_DEBUG << "TCPConnector connection in progress to " << addr.toIpPort();
            connecting();
            break;
            
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            // Retry-able errors
            LOG_DEBUG << "TCPConnector connection failed with retryable error: " 
                      << strerror_tl(savedErrno) << " to " << addr.toIpPort();
            retry();
            break;
            
        default:
            // Fatal errors
            LOG_ERROR << "TCPConnector connection failed with fatal error: " 
                      << strerror_tl(savedErrno) << " to " << addr.toIpPort();
            handleConnectError();
            break;
    }
    
    return ret;
}

void TCPConnector::connecting()
{
    this->loop()->assertInLoopThread();

    HCHECK(state_ == Connecting) << "TCPConnector state must be Connecting";

    auto sharedThis = shared_from_this();
    // Set up write callback to detect when connection completes
    Socket::setWriteCallback([sharedThis]() {
        sharedThis->handleConnectWrite();
    });

    Socket::setCloseCallback([sharedThis]() {
        sharedThis->handleConnectError();
    });

    Socket::setErrorCallback([sharedThis]() {
        sharedThis->handleConnectError();
    });
    
    this->enable();
}

void TCPConnector::handleConnectWrite()
{
    this->loop()->assertInLoopThread();
    // Note: Thread safety is ensured by runInLoop() calls in public methods
    
    LOG_DEBUG << "TCPConnector::handleConnectWrite for " << serverAddr_.toIpPort();
    
    if (state_ != Connecting) {
        LOG_WARN << "TCPConnector::handleConnectWrite called but state is not Connecting";
        return;
    }
    
    // Check if connection was successful
    int err = getSockError();
    if (err) {
        LOG_DEBUG << "TCPConnector connection failed in handleConnectWrite: " 
                  << strerror_tl(err) << " to " << serverAddr_.toIpPort();
        retry();
    } else {
        LOG_DEBUG << "TCPConnector connected successfully to " << serverAddr_.toIpPort();
        state_ = Connected;
        
        Socket::setWriteCallback(nullptr);
        Socket::setErrorCallback(nullptr);
        
        // move the socket handler out from TCPConnector
        // and call the new connection callback
        // This allows the user to take ownership of the IOHandler
        auto socketHandler = getSocketHandler();
        resetSocketHandler(nullptr);
        if (newConnectionCallback_) {
            newConnectionCallback_(std::make_shared<TCPConnection>(socketHandler));
        }
    }
}

void TCPConnector::handleConnectError()
{
    this->loop()->assertInLoopThread();
    
    if (state_ == Connected) {
        LOG_WARN << "TCPConnector::handleConnectError called but already connected";
        return;
    }
    
    int err = getSockError();
    LOG_DEBUG << "TCPConnector connection error: " << strerror_tl(err) 
              << " to " << serverAddr_.toIpPort();
    
    retry();
}

void TCPConnector::retry()
{
    this->loop()->assertInLoopThread();

    LOG_DEBUG << "TCPConnector::retry for " << serverAddr_.toIpPort();
    
    stop();//will happen in place, reset the socket handler
    
    if (maxRetries_ < 0 || currentRetries_ < maxRetries_) {
        currentRetries_++;
        if(!constantDelay_) {
            retryDelayMs_ = std::min(retryDelayMs_ * 2, 30000); //exponential grow but Cap at 30 seconds
        }
        
        
        LOG_DEBUG << "TCPConnector retrying connection to " << serverAddr_.toIpPort()
                  << " in " << retryDelayMs_ << "ms, " << (maxRetries_ < 0? -1: maxRetries_ - currentRetries_) << " retries left";
        
        auto sharedThis = shared_from_this();
        // Schedule retry
        loop()->addTimer([sharedThis]() {
            if (sharedThis->state_ == Disconnected) { // Only retry if still disconnected
                sharedThis->connect(sharedThis->serverAddr_);
            }
        }, addTime(Timestamp::now(), retryDelayMs_ / 1000.0));

        if (retryCallback_) {
            retryCallback_();
        }

    } else {
        LOG_DEBUG << "TCPConnector exhausted all retries for " << serverAddr_.toIpPort();
        
        if (failedCallback_) {
            failedCallback_();
        }
    }
}
