#ifndef NETWORK_NETCLIENT_H
#define NETWORK_NETCLIENT_H

#include <queue>
#include "Core/List.h"
#include "Os/TcpSocket.h"

class Connection {
public:
    Connection();
    virtual ~Connection();

    void SendRaw(const u8* data, u32 size);

    bool IsConnected();
    bool IsConnecting();
    bool Connect(u32 address, u16 port);

    void CheckConnect();
    void DoReads();
    void DoWrites();

    int GetSocket() const;

    LIST_LINK(Connection) m_link;

private:
    virtual void HandleMessage(u8* data, u32 size) = 0;
    virtual void HandleConnectSuccess() = 0;
    virtual void HandleConnectFailure() = 0;
    virtual void HandleDisconnect() = 0;

private:
    struct Message {
        u8* data;
        u32 size;
        u32 sizeBytesSent;
        u32 dataBytesSent;
    };

    void Disconnect();

    TcpSocket m_socket;
    std::queue<Message> m_messageQueue;
    std::vector<u8> m_readBuffer;
    u32 m_flags;
};

class NetClient {
public:
    NetClient();
    ~NetClient();

    void Update();

    // The NetClient class takes ownership of the Connection object and will
    // delete it in the NetClient destructor.
    void AddConnection(Connection* conn);

private:
    NetClient(const NetClient&);
    NetClient& operator=(const NetClient&);

    LIST_DECLARE(Connection, m_link) m_connections;
};

#endif // NETWORK_NETCLIENT_H
