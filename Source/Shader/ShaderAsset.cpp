#include "Shader/ShaderAsset.h"
#include <stdlib.h>

#include "GpuDevice/GpuDeferredDeletionQueue.h"

ShaderAsset::ShaderAsset(GpuDevice* device, const char* data, size_t length)
    : m_device(device)
    , m_shaderProgram(0)
{
    m_shaderProgram = device->ShaderProgramCreate(data, length);
}

ShaderAsset::~ShaderAsset()
{
    m_device->ShaderProgramDestroy(m_shaderProgram);
}

#ifdef ASSET_REFRESH
void ShaderAsset::Refresh(GpuDeferredDeletionQueue& deletionQ,
                          const char* data, size_t length)
{
    deletionQ.AddShaderProgram(m_shaderProgram);
    m_shaderProgram = m_device->ShaderProgramCreate(data, length);
    MarkRefreshed();
}
#endif

GpuShaderProgramID ShaderAsset::GetGpuShaderProgramID() const
{
    return m_shaderProgram;
}

ShaderAssetFactory::ShaderAssetFactory(GpuDevice* device
#ifdef ASSET_REFRESH
                                       , GpuDeferredDeletionQueue& deletionQ
#endif
)
    : m_device(device)
#ifdef ASSET_REFRESH
    , m_deletionQ(deletionQ)
#endif
{}

void* ShaderAssetFactory::Allocate(u32 size)
{
    return malloc(size);
}

ShaderAsset* ShaderAssetFactory::Create(u8* data, u32 size, const char* path)
{
    ShaderAsset* asset = new ShaderAsset(m_device, (const char*)data, (size_t)size);
    free(data);
    return asset;
}

#ifdef ASSET_REFRESH
void ShaderAssetFactory::Refresh(ShaderAsset* asset, u8* data, u32 size, const char* path)
{
    asset->Refresh(m_deletionQ, (const char*)data, (size_t)size - 1);
    free(data);
}
#endif
