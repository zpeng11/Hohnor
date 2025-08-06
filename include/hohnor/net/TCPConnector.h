#pragma once
#include "hohnor/net/Socket.h"
#include "hohnor/net/InetAddress.h"

namespace Hohnor
{
    class IOHandler;
    /*
    * TCPConnector is a class that handles the connection to a TCP server.
    * It uses the Socket class to manage the socket and provides a way to connect to a server.
    */
    class TCPConnector : public Socket{
    public:
        typedef std::function<void (std::shared_ptr<IOHandler>)> NewConnectionCallback;
        typedef std::function<void ()> RetryConnectionCallback;
        typedef std::function<void ()> FailedConnectionCallback;
        // Constructor that initializes the connector with an EventLoop and connection parameters
        TCPConnector(EventLoop* loop, const InetAddress& addr);
        
        void setNewConnectionCallback(NewConnectionCallback cb) { newConnectionCallback_ = std::move(cb); }
        void setRetryConnectionCallback(RetryConnectionCallback cb) { retryCallback_ = std::move(cb); }
        void setFailedConnectionCallback(FailedConnectionCallback cb) { failedCallback_ = std::move(cb); }
        void setRetryDelay(int delayMs) { retryDelayMs_ = delayMs; }
        void setRetries(int retries) { retries_ = retries; }

        void start(int retryDelayMs = 500, int retries = 0); // Start the connection process
        void stop();  // Stop the connection process
        void restart(); // Restart the connection process

        InetAddress getServerAddr() const { return serverAddr_; }

        enum State { Disconnected, Connecting, Connected };
    
    private:
        InetAddress serverAddr_;
        int retryDelayMs_;
        int retries_;
        NewConnectionCallback newConnectionCallback_;
        RetryConnectionCallback retryCallback_;
        FailedConnectionCallback failedCallback_;
        State state_;

        //Hide setCallbacks from Socket into private, we don't need them
        void setReadCallback(ReadCallback cb);
        void setWriteCallback(WriteCallback cb);
        void setCloseCallback(CloseCallback cb);
        void setErrorCallback(ErrorCallback cb);
        int connect(const InetAddress &addr, bool nonBlocking);
        void enable();
        void disable();

        // connecting in progress, e.g. EINPROGRESS, need to setup writecallback and enable handler
        void connecting();
        // handle the socket events, connection established, call newConnectionCallback
        void handleConnectWrite();
        // handle the socket events, connection failed, call failedConnectionCallback
        void handleConnectError();
        // count down retries and call retryCallback and restart if retries left.
        void retry();

    };

}