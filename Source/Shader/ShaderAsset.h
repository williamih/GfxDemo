#ifndef SHADER_SHADERASSET_H
#define SHADER_SHADERASSET_H

#include "Asset/Asset.h"
#include "GpuDevice/GpuDevice.h"

class GpuDeferredDeletionQueue;

class ShaderAsset : public Asset {
public:
    ShaderAsset(GpuDevice* device, const char* data, size_t length);
    virtual ~ShaderAsset();

#ifdef ASSET_REFRESH
    void Refresh(GpuDeferredDeletionQueue& deletionQ,
                 const char* data, size_t length);
#endif

    GpuShaderProgramID GetGpuShaderProgramID() const;
private:
    ShaderAsset(const ShaderAsset&);
    ShaderAsset& operator=(const ShaderAsset&);

    GpuDevice* m_device;
    GpuShaderProgramID m_shaderProgram;
};

class ShaderAssetFactory : public AssetFactory<ShaderAsset> {
public:
#ifdef ASSET_REFRESH
    ShaderAssetFactory(GpuDevice* device, GpuDeferredDeletionQueue& deletionQ);
#else
    explicit ShaderAssetFactory(GpuDevice* device);
#endif

    virtual std::shared_ptr<ShaderAsset> Create(const char* path, FileLoader& loader);

#ifdef ASSET_REFRESH
    virtual void Refresh(ShaderAsset* asset, const char* path, FileLoader& loader);
#endif

private:
    GpuDevice* m_device;
#ifdef ASSET_REFRESH
    GpuDeferredDeletionQueue& m_deletionQ;
#endif
};

#endif // SHADER_SHADERASSET_H
