#ifndef OS_TCPSOCKET_H
#define OS_TCPSOCKET_H

#include <stddef.h>
#include "Core/Types.h"

class TcpSocket {
public:
    enum BlockingMode {
        BLOCKING,
        NONBLOCKING,
    };

    enum SocketResult {
        SUCCESS,
        FAILURE,
        WOULDBLOCK,
    };

    enum PollConnectionResult {
        CONNECTION_SUCCEEDED,
        CONNECTION_IN_PROGRESS,
        CONNECTION_ERROR,
    };

    typedef int OsHandle;

    explicit TcpSocket(BlockingMode blockingMode);
    ~TcpSocket();

    BlockingMode GetBlockingMode() const;

    SocketResult Connect(u32 address, u16 port);
    void Disconnect();
    PollConnectionResult PollNonblockingConnect();
    bool IsConnectionInProgress() const;
    bool IsConnected() const;

    SocketResult Send(const void* data, size_t bytes);
    SocketResult Recv(void* buf, size_t bufLen, size_t* received);
private:
    TcpSocket(const TcpSocket&);
    TcpSocket& operator=(const TcpSocket&);

    void Create();

    OsHandle m_handle;
    u32 m_flags;
};

#endif // OS_TCPSOCKET_H
