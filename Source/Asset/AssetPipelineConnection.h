#ifndef ASSETPIPELINECONNECTION_H
#define ASSETPIPELINECONNECTION_H

#include <vector>
#include "Network/NetClient.h"

class ShaderCache;
struct AssetPipelineMsgCompiled;

class AssetPipelineConnection : public Connection {
public:
    explicit AssetPipelineConnection(ShaderCache& shaderCache);

    void Connect();

private:
    virtual void HandleMessage(u8* data, u32 size);
    virtual void HandleConnectSuccess();
    virtual void HandleConnectFailure();
    virtual void HandleDisconnect();

    void HandleAssetCompiled(AssetPipelineMsgCompiled& msg);

    ShaderCache& m_shaderCache;
};

#endif // ASSETPIPELINECONNECTION_H
