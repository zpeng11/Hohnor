#pragma once
#include "hohnor/net/Socket.h"
#include "hohnor/net/InetAddress.h"
#include <memory>

namespace Hohnor
{
    class IOHandler;
    /*
    * TCPConnector is a class that handles the connection to a TCP server.
    * It uses the Socket class to manage the socket and provides a way to connect to a server.
    */
    class TCPConnector : public Socket, public std::enable_shared_from_this<TCPConnector>
    {
    public:
        typedef std::function<void (std::shared_ptr<IOHandler>)> NewConnectionCallback;
        typedef std::function<void ()> RetryConnectionCallback;
        typedef std::function<void ()> FailedConnectionCallback;
        // Constructor that initializes the connector with an EventLoop and connection parameters
        TCPConnector(std::shared_ptr<EventLoop> loop, const InetAddress& addr);

        //will be called when the connection is established
        void setNewConnectionCallback(NewConnectionCallback cb);
        //will be called when the connection is retried
        void setRetryConnectionCallback(RetryConnectionCallback cb);
        //will be called when the connection fails after all retries
        void setFailedConnectionCallback(FailedConnectionCallback cb);

        //By default the Connector uses exponential delay from 500ms, you can set a constant delay
        //This is useful for testing or when you want to control the retry timing
        //Note: This will override the exponential backoff behavior
        //and use a constant delay for retries.
        void setRetryConstantDelay(int delayMs) { retryDelayMs_ = delayMs; constantDelay_ = true; }
        //Set the number of retries before giving up
        //If retries <= 0, it will retry indefinitely until success or stopped
        //If retries > 0, it will retry that many times before giving up
        void setRetries(int retries) { maxRetries_ = retries; }

        //Start the connection process, thread safe, can be called multiple times to create multiple connectors.
        void start(); 
        //Stop current connector's attempt to connect, thread safe.
        void stop();  

        InetAddress getServerAddr() const { return serverAddr_; }

        enum State { Disconnected, Connecting, Connected };
    
    private:
        InetAddress serverAddr_;
        static constexpr int DefaultRetryDelayMs = 500; // Default retry delay in milliseconds
        bool constantDelay_;
        int retryDelayMs_;
        int maxRetries_;
        int currentRetries_;
        NewConnectionCallback newConnectionCallback_;
        RetryConnectionCallback retryCallback_;
        FailedConnectionCallback failedCallback_;
        State state_;

        //Hide setCallbacks from Socket into private, we don't need them
        using Socket::setReadCallback;
        using Socket::setWriteCallback;
        using Socket::setCloseCallback;
        using Socket::setErrorCallback;
        using Socket::enable;
        using Socket::disable;
        using Socket::isEnabled;

        int connect(const InetAddress& addr);

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