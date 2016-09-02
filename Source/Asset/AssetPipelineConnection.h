#ifndef ASSETPIPELINECONNECTION_H
#define ASSETPIPELINECONNECTION_H

#include <vector>
#include "Network/NetClient.h"

struct AssetPipelineMsgCompiled;

class AssetPipelineConnection : public Connection {
public:
    AssetPipelineConnection();

    void Connect();

private:
    virtual void HandleMessage(u8* data, u32 size);
    virtual void HandleConnectSuccess();
    virtual void HandleConnectFailure();
    virtual void HandleDisconnect();

    void HandleAssetCompiled(AssetPipelineMsgCompiled& msg);
};

#endif // ASSETPIPELINECONNECTION_H
