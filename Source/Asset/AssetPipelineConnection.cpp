#include "Asset/AssetPipelineConnection.h"
#include <stdio.h>
#include "Core/Endian.h"

const u32 MSG_ASSET_COMPILED = 1;

struct AssetPipelineMsgCompiled {
    static const u32 MIN_SIZE = 5;

    u32 msgLen;
    char path[1];
};

static u32 Address(u32 a, u32 b, u32 c, u32 d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

const u16 PORT = 6789;
const u32 LOCALHOST = Address(127, 0, 0, 1);

AssetPipelineConnection::AssetPipelineConnection()
{}

void AssetPipelineConnection::Connect()
{
    Connection::Connect(LOCALHOST, PORT);
}

void AssetPipelineConnection::HandleMessage(u8* data, u32 size)
{
    if (size < 4)
        return;

    u32 type;
    memcpy(&type, data, 4);
    data += 4;
    type = EndianSwapLE32(type);

    const u32 payloadSize = size - sizeof type;

    switch (type) {
        case MSG_ASSET_COMPILED:
            if (payloadSize >= AssetPipelineMsgCompiled::MIN_SIZE)
                HandleAssetCompiled(*(AssetPipelineMsgCompiled*)data);
            break;
        default:
            break;
    }
}

void AssetPipelineConnection::HandleConnectSuccess()
{
    printf("Connected to asset pipeline.\n");
}

void AssetPipelineConnection::HandleConnectFailure()
{
    printf("Failed to connect to the asset pipeline.\n");
}

void AssetPipelineConnection::HandleDisconnect()
{
    printf("Disconnected from the asset pipeline.\n");
}

void AssetPipelineConnection::HandleAssetCompiled(AssetPipelineMsgCompiled& msg)
{
    printf("Recompiled file %s\n", msg.path);
}
