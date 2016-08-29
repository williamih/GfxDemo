#include "Os/TcpSocket.h"

#include <string.h> // for memset
#include <unistd.h> // for close()
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "Core/Macros.h"

static void ProcessSocketError()
{
    switch (errno) {
        // Errors that should not occur -- indicating a programming error in
        // this file.
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
        case EBADF:
        case EDESTADDRREQ:
        case EMSGSIZE:
        case ENOTCONN:
        case ENOTSOCK:
        case EOPNOTSUPP:
        case EPIPE:
        case EALREADY:
        case EINPROGRESS:
        case EISCONN:
        case EINVAL:
        case ELOOP:
        case ENAMETOOLONG: // not sure if this error should be in this category?
        case EFAULT:
        // Errors relating to AF_UNIX sockets -- we don't use these, so these
        // errors should not occur
        case ENOENT:
        case ENOTDIR:
            FATAL("TcpSocket: programming error: %s", strerror(errno));
            break;

        // Errors relating to the address
        case EAFNOSUPPORT:
            FATAL("TcpSocket: address invalid or unavailable");
            break;

        case EADDRNOTAVAIL:
            break;

        // Too many file descriptors open
        case EMFILE:
        case ENFILE:
            break;

        // Errors indicating a problem with the network or system -- these are
        // not under our control.
        case EACCES:
        case EIO:
        case EADDRINUSE:
        case ECONNRESET:
        case ECONNREFUSED:
        case EHOSTUNREACH:
        case ENETDOWN:
        case ENETUNREACH:
        case ENOBUFS:
        case ENOMEM:
        case EPROTOTYPE:
        case ETIMEDOUT:
        case EPROTONOSUPPORT: // not sure whether this is in the right place
            break;

        default:
            FATAL("TcpSocket: error: %s", strerror(errno));
            break;
    }
}

TcpSocket::TcpSocket()
    : m_handle(-1)
{}

TcpSocket::~TcpSocket()
{
    Disconnect();
}

bool TcpSocket::Connect(u32 address, u16 port)
{
    ASSERT(!IsConnected());
    Create();

    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(address);
    for (;;) {
        if (connect(m_handle, (sockaddr*)&addr, sizeof addr) != 0) {
            if (errno == EINTR)
                continue;
            ProcessSocketError();
            Disconnect();
            return false;
        }
        break;
    }

    return true;
}

void TcpSocket::Disconnect()
{
    if (m_handle != -1) {
        if (close(m_handle) == -1)
            FATAL("close");
        m_handle = -1;
    }
}

bool TcpSocket::IsConnected() const
{
    return m_handle != -1;
}

bool TcpSocket::Send(const void* data, size_t bytes)
{
    ASSERT(IsConnected());
    size_t remaining = bytes;
    const void* p = data;
    while (remaining > 0) {
        ssize_t sent = send(m_handle, p, remaining, 0);
        if (sent == -1) {
            if (errno == EINTR)
                continue;
            ProcessSocketError();
            Disconnect();
            return false;
        }
        remaining -= (size_t)sent;
        p = (const u8*)p + (size_t)sent;
    }
    return true;
}

bool TcpSocket::Recv(void* buf, size_t bufLen, size_t* received)
{
    ASSERT(IsConnected());
    ssize_t ret;
    for (;;) {
        ret = recv(m_handle, buf, bufLen, 0);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            ProcessSocketError();
            Disconnect();
            return false;
        }
        break;
    }
    if (ret == 0)
        Disconnect(); // orderly shutdown on remote end
    if (received)
        *received = (size_t)ret;
    return true;
}

void TcpSocket::Create()
{
    ASSERT(!IsConnected());
    m_handle = socket(PF_INET, SOCK_STREAM, 0);
    if (m_handle == -1)
        FATAL("Failed to create socket: %s", strerror(errno));
}
