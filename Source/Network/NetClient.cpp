#include "Network/NetClient.h"

#include <stdlib.h>

#include "Core/Endian.h"
#include "Os/SocketOsFunctions.h"

const u32 FLAG_CONNECTED = 1;
const u32 FLAG_CONNECTING = 2;

Connection::Connection()
    : m_socket()
    , m_link()
    , m_messageQueue()
    , m_flags(0)
{
    m_socket.SetBlockingMode(TcpSocket::NONBLOCKING);
}

Connection::~Connection()
{
    while (!m_messageQueue.empty()) {
        free(m_messageQueue.front().data);
        m_messageQueue.pop();
    }
}

static bool TrySend(
    TcpSocket& socket,
    const u8* data,
    u32 size,
    u32* sizeBytesSent,
    u32* dataBytesSent)
{
    ASSERT(sizeBytesSent);
    ASSERT(dataBytesSent);

    size_t sent;

    if (*sizeBytesSent < 4) {
        u32 swappedSize = EndianSwapLE32(size);
        const u8* ptr = (u8*)&swappedSize + *sizeBytesSent;
        if (!socket.Send(ptr, 4 - *sizeBytesSent, &sent))
            return false;
        *sizeBytesSent += (u32)sent;
    }

    ASSERT(*sizeBytesSent <= 4);

    if (*sizeBytesSent == 4) {
        const u8* ptr = data + *dataBytesSent;
        if (!socket.Send(ptr, size - *dataBytesSent, &sent))
            return false;
        *dataBytesSent += (u32)sent;
    }

    return true;
}

void Connection::SendRaw(const u8* data, u32 size)
{
    u32 sizeBytesSent = 0;
    u32 dataBytesSent = 0;

    if (!TrySend(m_socket, data, size, &sizeBytesSent, &dataBytesSent)) {
        Disconnect();
        return;
    }

    if (dataBytesSent < size) {
        Message message;
        message.data = (u8*)malloc(size);
        memcpy(message.data, data, size);
        message.size = size;
        message.sizeBytesSent = sizeBytesSent;
        message.dataBytesSent = dataBytesSent;

        m_messageQueue.push(message);
    }
}

static inline u32 ParseLE32(const u8* ptr)
{
    return (u32)ptr[0] | ((u32)ptr[1] << 8) | ((u32)ptr[2] << 16) |
           ((u32)ptr[3] << 24);
}

bool Connection::IsConnected()
{
    return (m_flags & FLAG_CONNECTED) != 0;
}

bool Connection::IsConnecting()
{
    return (m_flags & FLAG_CONNECTING) != 0;
}

bool Connection::Connect(u32 address, u16 port)
{
    TcpSocket::SocketResult result = m_socket.Connect(address, port);
    switch (result) {
        case TcpSocket::SUCCESS:
            m_flags |= FLAG_CONNECTED;
            HandleConnectSuccess();
            return true;
        case TcpSocket::FAILURE:
            HandleConnectFailure();
            return false;
        case TcpSocket::WOULDBLOCK:
            m_flags |= FLAG_CONNECTING;
            return true;
    }
}

void Connection::CheckConnect()
{
    ASSERT(m_flags & FLAG_CONNECTING);
    ASSERT(!(m_flags & FLAG_CONNECTED));
    m_flags &= ~(FLAG_CONNECTING);
    bool result;
    m_socket.CheckErrorStatus(&result);
    if (result) {
        // Connected successfully
        m_flags |= FLAG_CONNECTED;
        HandleConnectSuccess();
    } else {
        // Failed to connect
        HandleConnectFailure();
    }
}

void Connection::DoReads()
{
    u8 buf[1024];
    size_t received;
    for (;;) {
        TcpSocket::SocketResult res = m_socket.Recv(buf, sizeof buf, &received);
        if (res == TcpSocket::WOULDBLOCK)
            break;
        if (res == TcpSocket::FAILURE) {
            Disconnect();
            break;
        }
        // TODO: Redesign TcpSocket::Recv() so that we don't have to do this
        // additional check.
        if (received == 0) {
            Disconnect();
            break;
        }

        const u8* ptr = buf;
        const u8* const end = buf + received;

        while (ptr != end) {
            const size_t bytesRemaining = end - ptr;

            u32 toCopy;
            if (m_readBuffer.size() < 4) {
                toCopy = (u32)std::min(bytesRemaining, 4 - m_readBuffer.size());
            } else {
                u32 messageSize = ParseLE32(&m_readBuffer[0]);
                toCopy = std::min((u32)bytesRemaining, messageSize);
            }

            const size_t bufferSize = m_readBuffer.size();
            m_readBuffer.resize(bufferSize + toCopy);
            memcpy(&m_readBuffer[bufferSize], ptr, toCopy);
            ptr += toCopy;

            if (m_readBuffer.size() >= 4) {
                u32 messageSize = ParseLE32(&m_readBuffer[0]);
                if (m_readBuffer.size() == 4 + messageSize) {
                    HandleMessage(&m_readBuffer[4], messageSize);
                    m_readBuffer.clear();
                }
            }
        }
    }
}

void Connection::DoWrites()
{
    ASSERT(IsConnected());
    while (!m_messageQueue.empty()) {
        Message message = m_messageQueue.front();
        m_messageQueue.pop();

        if (!TrySend(m_socket, message.data, message.size,
                     &message.sizeBytesSent, &message.dataBytesSent)) {
            Disconnect();
        }
        free(message.data);

        if (!IsConnected())
            break;
    }
}

int Connection::GetSocket() const
{
    return m_socket.GetOsHandle();
}

void Connection::Disconnect()
{
    m_socket.Disconnect();
    m_flags &= ~(FLAG_CONNECTED);
    HandleDisconnect();
}

NetClient::NetClient()
    : m_connections()
{}

NetClient::~NetClient()
{
    while (Connection* conn = m_connections.Head())
        delete conn;
}

void NetClient::Update()
{
    fd_set readSet, writeSet;
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    int nfds = 0;

    for (Connection* conn = m_connections.Head(); conn;
         conn = conn->m_link.Next()) {
        int socket = conn->GetSocket();
        if (socket > FD_SETSIZE)
            FATAL("Socket exceeds FD_SETSIZE");
        if (!conn->IsConnected() && !conn->IsConnecting())
            continue;
        if (conn->IsConnected())
            FD_SET(socket, &readSet);
        FD_SET(socket, &writeSet);
        if (socket > nfds)
            nfds = socket;
    }
    if (nfds == 0)
        return;
    ++nfds;

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int ret = select(nfds, &readSet, &writeSet, NULL, &tv);

    if (ret == -1)
        FATAL("select");

    if (ret == 0)
        return; // nothing happened

    for (Connection* conn = m_connections.Head(); conn;
         conn = conn->m_link.Next()) {
        int socket = conn->GetSocket();
        if (FD_ISSET(socket, &readSet)) {
            conn->DoReads();
        } else if (FD_ISSET(socket, &writeSet)) {
            if (conn->IsConnected()) {
                conn->DoWrites();
            } else {
                conn->CheckConnect();
            }
        }
    }
}

void NetClient::AddConnection(Connection* conn)
{
    m_connections.InsertTail(conn);
}
