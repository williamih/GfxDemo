#include "Os/TcpSocket.h"

#include <string.h> // for memset
#include <unistd.h> // for close()
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>

#include "Core/Macros.h"

const u32 FLAG_NONBLOCKING = 1;
const u32 FLAG_CONNECTED = 2;
const u32 FLAG_CONNECTION_IN_PROGRESS = 4;

static void ProcessSocketError(int error)
{
    switch (error) {
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
            FATAL("TcpSocket: programming error: %s", strerror(error));
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
            FATAL("TcpSocket: error: %s", strerror(error));
            break;
    }
}

TcpSocket::TcpSocket(BlockingMode blockingMode)
    : m_handle(-1)
    , m_flags((blockingMode == NONBLOCKING) ? FLAG_NONBLOCKING : 0)
{
    if (blockingMode == NONBLOCKING) {
        int flags = fcntl(m_handle, F_GETFL, 0);
        if (flags < 0)
            FATAL("fcntl");
        flags |= O_NONBLOCK;
        if (fcntl(m_handle, F_SETFL, flags) != 0)
            FATAL("fcntl");
    }
}

TcpSocket::~TcpSocket()
{
    Disconnect();
}

TcpSocket::BlockingMode TcpSocket::GetBlockingMode() const
{
    return (m_flags & FLAG_NONBLOCKING) ? NONBLOCKING : BLOCKING;
}

TcpSocket::SocketResult TcpSocket::Connect(u32 address, u16 port)
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
            // TODO: Do we need to check for EWOULDBLOCK as well?
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                m_flags |= FLAG_CONNECTION_IN_PROGRESS;
                return WOULDBLOCK;
            }
            ProcessSocketError(errno);
            Disconnect();
            return FAILURE;
        }
        break;
    }

    m_flags |= FLAG_CONNECTED;
    return SUCCESS;
}

void TcpSocket::Disconnect()
{
    if (m_handle != -1) {
        if (close(m_handle) == -1)
            FATAL("close");
        m_handle = -1;
    }
    m_flags &= ~(FLAG_CONNECTED);
}

TcpSocket::PollConnectionResult TcpSocket::PollNonblockingConnect()
{
    ASSERT(m_flags & FLAG_CONNECTION_IN_PROGRESS);

    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(m_handle, &writefds);

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int ret = select(1, NULL, &writefds, NULL, &tv);
    if (ret == 0)
        return CONNECTION_IN_PROGRESS;

    if (ret == -1)
        FATAL("select: %s\n", strerror(errno));

    m_flags &= ~(FLAG_CONNECTION_IN_PROGRESS);

    int error = 0;
    socklen_t len = sizeof error;
    int retval = getsockopt(m_handle, SOL_SOCKET, SO_ERROR, &error, &len);
    if (retval != 0)
        FATAL("getsockopt: %s", strerror(retval));

    if (error == 0) {
        m_flags |= FLAG_CONNECTED;
        return CONNECTION_SUCCEEDED;
    } else {
        ProcessSocketError(error);
        return CONNECTION_ERROR;
    }
}

bool TcpSocket::IsConnectionInProgress() const
{
    return (m_flags & FLAG_CONNECTION_IN_PROGRESS) != 0;
}

bool TcpSocket::IsConnected() const
{
    return (m_flags & FLAG_CONNECTED) != 0;
}

TcpSocket::SocketResult TcpSocket::Send(const void* data, size_t bytes)
{
    ASSERT(IsConnected());
    size_t remaining = bytes;
    const void* p = data;
    while (remaining > 0) {
        ssize_t sent = send(m_handle, p, remaining, 0);
        if (sent == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return WOULDBLOCK;
            ProcessSocketError(errno);
            Disconnect();
            return FAILURE;
        }
        remaining -= (size_t)sent;
        p = (const u8*)p + (size_t)sent;
    }
    return SUCCESS;
}

TcpSocket::SocketResult TcpSocket::Recv(void* buf, size_t bufLen, size_t* received)
{
    ASSERT(IsConnected());
    ssize_t ret;
    for (;;) {
        ret = recv(m_handle, buf, bufLen, 0);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return WOULDBLOCK;
            ProcessSocketError(errno);
            Disconnect();
            return FAILURE;
        }
        break;
    }
    if (ret == 0)
        Disconnect(); // orderly shutdown on remote end
    if (received)
        *received = (size_t)ret;
    return SUCCESS;
}

void TcpSocket::Create()
{
    ASSERT(!IsConnected());
    m_handle = socket(PF_INET, SOCK_STREAM, 0);
    if (m_handle == -1)
        FATAL("Failed to create socket: %s", strerror(errno));
}
