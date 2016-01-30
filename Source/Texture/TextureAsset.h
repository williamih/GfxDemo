#ifndef TEXTURE_TEXTUREASSET_H
#define TEXTURE_TEXTUREASSET_H

#include "Asset/Asset.h"

#include "GpuDevice/GpuDevice.h"

class GpuDeferredDeletionQueue;
class DDSFile;

class TextureAsset : public Asset {
public:
    TextureAsset(GpuDevice* device, DDSFile& file);
    virtual ~TextureAsset();

#ifdef ASSET_REFRESH
    void Refresh(GpuDeferredDeletionQueue& deletionQ, DDSFile& file);
#endif

    GpuTextureID GetGpuTextureID() const;
private:
    TextureAsset(const TextureAsset&);
    TextureAsset& operator=(const TextureAsset&);

    GpuDevice* m_device;
    GpuTextureID m_texture;
};

class TextureAssetFactory : public AssetFactory<TextureAsset> {
public:
#ifdef ASSET_REFRESH
    TextureAssetFactory(GpuDevice* device, GpuDeferredDeletionQueue& deletionQ);
#else
    explicit TextureAssetFactory(GpuDevice* device);
#endif

    virtual void* Allocate(u32 size);

    virtual TextureAsset* Create(u8* data, u32 size, const char* path);

#ifdef ASSET_REFRESH
    virtual void Refresh(TextureAsset* asset, u8* data, u32 size, const char* path);
#endif

private:
    GpuDevice* m_device;
#ifdef ASSET_REFRESH
    GpuDeferredDeletionQueue& m_deletionQ;
#endif
};

#endif // TEXTURE_TEXTUREASSET_H
