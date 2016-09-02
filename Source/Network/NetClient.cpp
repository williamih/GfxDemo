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
{}

Connection::~Connection()
{
    while (!m_messageQueue.empty()) {
        free(m_messageQueue.front().data);
        m_messageQueue.pop();
    }
}

void Connection::SendRaw(const u8* data, u32 size)
{
    Message message;
    message.data = (u8*)malloc(size);
    memcpy(message.data, data, size);
    message.size = size;

    m_messageQueue.push(message);
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
    m_socket.SetBlockingMode(TcpSocket::NONBLOCKING);
    TcpSocket::SocketResult result = m_socket.Connect(address, port);
    switch (result) {
        case TcpSocket::SUCCESS:
            m_flags |= FLAG_CONNECTED;
            m_socket.SetBlockingMode(TcpSocket::BLOCKING);
            HandleConnectSuccess();
            return true;
        case TcpSocket::FAILURE:
            m_socket.SetBlockingMode(TcpSocket::BLOCKING);
            HandleConnectFailure();
            return false;
        case TcpSocket::WOULDBLOCK:
            m_flags |= FLAG_CONNECTING;
            // We leave the socket in nonblocking mode until the connection
            // is established.
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
        m_socket.SetBlockingMode(TcpSocket::BLOCKING);
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
    if (m_socket.Recv(buf, sizeof buf, &received) == TcpSocket::FAILURE) {
        Disconnect();
        return;
    }
    // TODO: Redesign TcpSocket::Recv() so that we don't have to do this
    // additional check.
    if (received == 0) {
        Disconnect();
        return;
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

void Connection::DoWrites()
{
    if (!IsConnected())
        return;
    while (!m_messageQueue.empty()) {
        Message message = m_messageQueue.front();
        m_messageQueue.pop();

        u32 size = EndianSwapLE32(message.size);
        for (;;) {
            if (!m_socket.Send(&size, 4, NULL)) {
                Disconnect();
                break;
            }
            if (!m_socket.Send(message.data, message.size, NULL)) {
                Disconnect();
                break;
            }
            break;
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
    for (Connection* conn = m_connections.Head(); conn;
         conn = conn->m_link.Next()) {
        conn->DoWrites();
    }

    for (;;) {
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
            if (conn->IsConnected()) {
                FD_SET(socket, &readSet);
            } else {
                FD_SET(socket, &writeSet);
            }
            if (socket > nfds)
                nfds = socket;
        }
        if (nfds == 0)
            break;
        ++nfds;

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        int ret = select(nfds, &readSet, &writeSet, NULL, &tv);

        if (ret == -1)
            FATAL("select");

        if (ret == 0)
            break; // nothing happened

        for (Connection* conn = m_connections.Head(); conn;
             conn = conn->m_link.Next()) {
            int socket = conn->GetSocket();
            if (FD_ISSET(socket, &readSet))
                conn->DoReads();
            else if (FD_ISSET(socket, &writeSet))
                conn->CheckConnect();
        }
    }
}

void NetClient::AddConnection(Connection* conn)
{
    m_connections.InsertTail(conn);
}
