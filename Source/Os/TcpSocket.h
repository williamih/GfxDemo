#ifndef OS_TCPSOCKET_H
#define OS_TCPSOCKET_H

#include <stddef.h>
#include "Core/Types.h"

class TcpSocket {
public:
    typedef int OsHandle;

    TcpSocket();
    ~TcpSocket();

    bool Connect(u32 address, u16 port);
    void Disconnect();
    bool IsConnected() const;

    bool Send(const void* data, size_t bytes);
    bool Recv(void* buf, size_t bufLen, size_t* received);
private:
    TcpSocket(const TcpSocket&);
    TcpSocket& operator=(const TcpSocket&);

    void Create();

    OsHandle m_handle;
};

#endif // OS_TCPSOCKET_H
