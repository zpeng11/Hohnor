#include "hohnor/net/TCPConnection.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/core/IOHandler.h"
#include "hohnor/log/Logging.h"
#include "hohnor/io/FdUtils.h"
#include "hohnor/common/StringPiece.h"
#include "hohnor/common/Buffer.h"
#include "hohnor/time/Timestamp.h"
#include <errno.h>
#include <unistd.h>
#include <netinet/tcp.h>

using namespace Hohnor;

TCPConnection::TCPConnection(std::shared_ptr<IOHandler> handler)
    : Socket(handler, handler->loop()),
      writing_(false),
      readBuffer_(),
      writeBuffer_(),
      highWaterMark_(64*1024*1024), // 64MB default high water mark
      highWaterMarkCallback_(),
      readCompleteCallback_(),
      writeCompleteCallback_(),
      readStopCondition_(),
      closeCallback_(),
      errorCallback_()
{
    LOG_DEBUG << "TCPConnection::ctor at " << this
              << " fd=" << handler->fd();

    // Set up the IOHandler callbacks using weak_ptr to avoid circular references
    getSocketHandler()->setReadCallback(std::bind(&TCPConnection::handleRead, this));
    getSocketHandler()->setWriteCallback(std::bind(&TCPConnection::handleWrite, this));
    getSocketHandler()->setCloseCallback(std::bind(&TCPConnection::handleClose, this));
    getSocketHandler()->setErrorCallback(std::bind(&TCPConnection::handleError, this));

    // Disable write events initially
    setWriteEvent(false);
    enable();
}

TCPConnection::~TCPConnection()
{
    LOG_DEBUG << "TCPConnection::dtor at " << this
              << " fd=" << getSocketHandler()->fd();

    if (getSocketHandler() && (!getSocketHandler()->isEnabled())) {
        getSocketHandler()->disable();
    }
}

// --- Callback Setters ---
void TCPConnection::setHighWaterMarkCallback(size_t highWaterMark, const HighWaterMarkCallback& cb)
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis, highWaterMark, cb]() {
        sharedThis->highWaterMark_ = highWaterMark;
        sharedThis->highWaterMarkCallback_ = cb;
    });
}

void TCPConnection::setReadCompleteCallback(const ReadCompleteCallback& cb)
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis, cb]() {
        sharedThis->readCompleteCallback_ = cb;
    });
}

void TCPConnection::setWriteCompleteCallback(const WriteCompleteCallback& cb)
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis, cb]() {
        sharedThis->writeCompleteCallback_ = cb;
    });
}

void TCPConnection::setCloseCallback(const CloseCallback& cb)
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis, cb]() {
        sharedThis->closeCallback_ = cb;
    });
}

void TCPConnection::setErrorCallback(const ErrorCallback& cb)
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis, cb]() {
        sharedThis->errorCallback_ = cb;
    });
}

// --- Read Operations ---
void TCPConnection::readRaw()
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis]() {
        sharedThis->readStopCondition_ = nullptr;
    });
}

void TCPConnection::readUntil(const std::string& delimiter)
{
    ReadStopCondition condition = [delimiter](Buffer& buffer) -> bool {
        const char* found = std::search<const char *, const char *>( (buffer.beginWrite() - buffer.lastWriteBytes()), 
                                                                    buffer.beginWrite(), delimiter.c_str(), 
                                                                    (delimiter.c_str() + delimiter.size()));
        return found != buffer.beginWrite();
    };
    readUntilCondition(condition);
}

void TCPConnection::readBytes(size_t length)
{
    ReadStopCondition condition = [length](Buffer& buffer) -> bool {
        return buffer.readableBytes() >= length;
    };
    readUntilCondition(condition);
}

void TCPConnection::readUntilCondition(ReadStopCondition condition)
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis, condition]() {
        sharedThis->readStopCondition_ = condition;
    });
}

// --- Write Operations ---
void TCPConnection::write(const void* data, int len)
{
    StringPiece message(static_cast<const char*>(data), len);
    write(message);
}

void TCPConnection::write(const StringPiece message)
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis, message]() {
        if (sharedThis->isClosed()) {
            LOG_ERROR << "TCPConnection::write called on a closed connection";
            return;
        }
        sharedThis->writeInLoop(message);
    });
}

void TCPConnection::write(const std::string& message)
{
    write(StringPiece(message));
}

void TCPConnection::write(Buffer* buffer)
{
    if (buffer->readableBytes() > 0) {
        write(buffer->readableSlice());
        buffer->retrieveAll();
    }
}

void TCPConnection::write()
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis]() {
        if(sharedThis->isClosed()){
            LOG_ERROR << "TCPConnection::write called on a closed connection";
            return;
        }
        sharedThis->getSocketHandler()->setWriteEvent(true);
    });
    enable(); // Ensure the read event is enabled
}

// --- Connection Management ---
void TCPConnection::shutdown()
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis]() {
        SocketFuncs::shutdownWrite(sharedThis->fd());
    });
}

void TCPConnection::forceClose()
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis]() {
        if (sharedThis->isClosed()) {
            LOG_ERROR << "TCPConnection::forceClose called on a closed connection";
            return;
        }
        sharedThis->disable();
        // sharedThis->Socket::resetSocketHandler();
    });
}

void TCPConnection::forceCloseWithDelay(double seconds)
{
    auto sharedThis = shared_from_this();
    loop()->addTimer([sharedThis]() {
        sharedThis->forceClose();
    }, addTime(Timestamp::now(), seconds));
}

// --- Flow Control ---
void TCPConnection::setTCPNoDelay(bool on)
{
    auto sharedThis = shared_from_this();
    loop()->runInLoop([sharedThis, on]() {
        if (sharedThis->isClosed()) {
            LOG_ERROR << "TCPConnection::setTCPNoDelay called on a closed connection";
            return;
        }
        int optval = on ? 1 : 0;
        ::setsockopt(sharedThis->getSocketHandler()->fd(), IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
    });

}

// --- TCP Info ---
struct tcp_info TCPConnection::getTCPInfo() const
{
    if (isClosed()) {
        LOG_ERROR << "TCPConnection::getTCPInfo called on a closed connection";
        return {};
    }
    return SocketFuncs::getTCPInfo(this->fd());
}

std::string TCPConnection::getTCPInfoStr() const
{
    if (isClosed()) {
        LOG_ERROR << "TCPConnection::getTCPInfoStr called on a closed connection";
        return {};
    }
    return SocketFuncs::getTCPInfoStr(this->fd());
}

// --- Internal Event Handlers ---
void TCPConnection::handleRead()
{
    loop()->assertInLoopThread();
    
    int savedErrno = 0;
    ssize_t n = readBuffer_.readFd(this->fd(), &savedErrno);
    
    if (n > 0) {
        // LOG_TRACE << "TCPConnection::handleRead fd [" << fd() << "] read " << n << " bytes to " << getTCPInfoStr();

        // Check if we should stop reading based on the current read mode
        if(isReadingUntilCondition() && readStopCondition_(readBuffer_)) //Reading until condition && condition meets
        {
            if (readCompleteCallback_) {
                readCompleteCallback_(shared_from_this());
            }
        } else if (!isReadingUntilCondition()) {//Reading raw, directly get into callback
            if (readCompleteCallback_) {
                readCompleteCallback_(shared_from_this());
            }
        }
        
        // Shrink buffer if it's getting too large and empty
        if (readBuffer_.readableBytes() == 0 && readBuffer_.capacity() > 1024*1024) {
            readBuffer_.shrink(0);
        }
    }
    else if (n == 0) {
        LOG_DEBUG << "TCPConnection::handleRead fd [" << fd() << "] connection closed by peer to " << getTCPInfoStr();
        handleClose();
    }
    else {
        LOG_ERROR << "TCPConnection::handleRead fd [" << fd() << "] error: " << strerror_tl(savedErrno) << getTCPInfoStr();
        errno = savedErrno;
        handleError();
    }
}

void TCPConnection::handleWrite()
{
    loop()->assertInLoopThread();
    
    if (!writing_) {
        LOG_WARN << "TCPConnection::handleWrite fd [" << fd() << "] not writing to " << getTCPInfoStr();
        return;
    }

    ssize_t n = ::write(fd(), writeBuffer_.peek(), writeBuffer_.readableBytes());
    if (n > 0) {
        writeBuffer_.retrieve(n);
        // LOG_TRACE << "TCPConnection::handleWrite fd [" << fd() << "] wrote " << n << " bytes to " << getTCPInfoStr();

        if (writeBuffer_.readableBytes() == 0) {
            // All data written
            setWriteEvent(false);
            writing_ = false;
            
            if (writeCompleteCallback_) {
                //Must put into queue, otherwise it may be called immediately and cause re-entrancy issues and oveerflow
                loop()->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
            
            // Shrink write buffer if it's getting too large and empty
            if (writeBuffer_.capacity() > 1024*1024) {
                writeBuffer_.shrink(0);
            }
        }
    }
    else {
        LOG_ERROR << "TCPConnection::handleWrite fd [" << fd() << "] error: " << strerror_tl(errno) << getTCPInfoStr();
        handleError();
    }
}

void TCPConnection::handleClose()
{   
    loop()->assertInLoopThread();

    LOG_DEBUG << "TCPConnection::handleClose fd [" << fd() << "] to "<< getTCPInfoStr();

    // Disable the handler to stop receiving events, but close usually comes with error,
    // So we let the disable() happen in queued function to avoid potential race conditions
    // with error handling
    if (!isClosed()) {
        auto weakThis = std::weak_ptr<TCPConnection>(shared_from_this());
        loop()->queueInLoop([weakThis]() {
            auto sharedThis = weakThis.lock();
            if (sharedThis) {
                sharedThis->getSocketHandler()->disable();
            }
        });
    }
    
    // Call the close callback if set
    if (closeCallback_) {
        closeCallback_();
    }
}

void TCPConnection::handleError()
{
    loop()->assertInLoopThread();
    
    int err = getSockError();
    LOG_ERROR << "TCPConnection::handleError fd [" << fd() << "] SO_ERROR = " << err << " " << strerror_tl(err);
    
    // Call the error callback if set
    if (errorCallback_) {
        errorCallback_();
    }
}

// --- Internal Helper Methods ---
void TCPConnection::writeInLoop(const StringPiece& message)
{
    loop()->assertInLoopThread();
    
    ssize_t nwrote = 0;
    size_t remaining = message.size();
    bool faultError = false;
    
    // If no data in output queue, try writing directly
    if (!writing_ && writeBuffer_.readableBytes() == 0) {
        nwrote = ::write(fd(), message.data(), message.size());
        if (nwrote >= 0) {
            remaining = message.size() - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop()->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR << "TCPConnection::writeInLoop fd [" << fd() << "] write error: " << strerror_tl(errno);
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    HCHECK(!faultError) << "TCPConnection::writeInLoop fd [" << fd() << "] write fault error";

    // If we couldn't write all data, append the rest to output buffer
    if (!faultError && remaining > 0) {
        size_t oldLen = writeBuffer_.readableBytes();
        
        // Check high water mark
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            highWaterMarkCallback_(shared_from_this());
        }
        
        writeBuffer_.append(message.data() + nwrote, remaining);
        if (!writing_) {
            setWriteEvent(true);
            writing_ = true;
        }
    }
}

void TCPConnection::writeInLoop(const void* data, size_t len)
{
    writeInLoop(StringPiece(static_cast<const char*>(data), len));
}

void TCPConnection::setWriteEvent(bool on)
{
    if (!isClosed()) {
        getSocketHandler()->setWriteEvent(on);
    }
}