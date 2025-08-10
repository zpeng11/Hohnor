#pragma once

#include "hohnor/common/NonCopyable.h"
#include "hohnor/common/Callbacks.h"
#include "hohnor/common/Buffer.h"
#include "hohnor/net/InetAddress.h"
#include "hohnor/net/Socket.h"
#include <memory>
#include <string>
#include <functional>

namespace Hohnor
{
    class EventLoop;
    class IOHandler;
    class TCPConnection;

    // Forward declaration for callback types
    typedef std::shared_ptr<TCPConnection> TCPConnectionPtr;
    typedef std::weak_ptr<TCPConnection> TCPConnectionWeakPtr;

    typedef std::function<void (TCPConnectionWeakPtr)> HighWaterMarkCallback;
    typedef std::function<void (TCPConnectionWeakPtr)> ReadCompleteCallback;
    typedef std::function<void (TCPConnectionWeakPtr)> WriteCompleteCallback;
    // A filter class that returns true to stop reading
    typedef std::function<bool (Buffer&)> ReadStopCondition;

    class TCPConnection : public Socket, public std::enable_shared_from_this<TCPConnection>
    {
    public:

        // Constructor - should only be called by TCPAcceptor or TCPConnector
        TCPConnection(std::shared_ptr<IOHandler> handler);

        ~TCPConnection();

        // --- Callback Setters ---
        void setHighWaterMarkCallback(size_t highWaterMark, const HighWaterMarkCallback& cb);
        void setReadCompleteCallback(const ReadCompleteCallback& cb);
        void setWriteCompleteCallback(const WriteCompleteCallback& cb);
        void setCloseCallback(const CloseCallback& cb);
        void setErrorCallback(const ErrorCallback& cb);

        // --- Connection Management ---

        // --- I/O Operations ---
        // Read operations
        void readRaw(); // Read raw data from read event
        void readUntil(const std::string& delimiter);      // read until delimiter
        void readBytes(size_t length);                    // read specified number of bytes
        void readUntilCondition(ReadStopCondition condition); // read until condition is met
        // Send data, copy provided data to write buffer
        void write(const void* data, int len);
        void write(const StringPiece message);
        void write(const std::string& message);
        void write(Buffer* buffer);
        void write();

        //Buffers
        Buffer& getReadBuffer() { return readBuffer_; }
        Buffer& getWriteBuffer() { return writeBuffer_; }

        // Shutdown write side of connection - thread safe
        void shutdown();
        // Force close connection - thread safe
        void forceClose();
        // Force close connection after delay - thread safe
        void forceCloseWithDelay(double seconds);

        bool isClosed() const{
            return getSocketHandler() == nullptr;
        }

        // --- Flow Control ---
        void setTCPNoDelay(bool on);

        // --- TCP Info ---
        struct tcp_info getTCPInfo() const;
        //Get Tcp information string. In case failed return empty string
        std::string getTCPInfoStr() const;

    private:
        bool isReadingUntilCondition() const { return readStopCondition_ != nullptr; }
        bool writing_;
        
        // Buffers for I/O
        Buffer readBuffer_;
        Buffer writeBuffer_;

        // High water mark for output buffer
        size_t highWaterMark_;
        
        // User callbacks
        HighWaterMarkCallback highWaterMarkCallback_;
        ReadCompleteCallback readCompleteCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        ReadStopCondition readStopCondition_;
        CloseCallback closeCallback_;
        ErrorCallback errorCallback_;

        // --- Internal Event Handlers ---
        void handleRead();
        void handleWrite();
        void handleClose();
        void handleError();
        
        // --- Internal Helper Methods ---
        void writeInLoop(const StringPiece& message);
        void writeInLoop(const void* data, size_t len);
        void setWriteEvent(bool on);

        //Hide methods
        using Socket::setReadCallback;
        using Socket::setWriteCallback;
        using Socket::enable;
        using Socket::disable;
        using Socket::isEnabled;
        
    };

} // namespace Hohnor