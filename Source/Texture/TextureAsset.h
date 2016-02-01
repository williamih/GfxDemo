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

    virtual std::shared_ptr<TextureAsset> Create(const char* path, FileLoader& loader);

#ifdef ASSET_REFRESH
    virtual void Refresh(TextureAsset* asset, const char* path, FileLoader& loader);
#endif

private:
    GpuDevice* m_device;
#ifdef ASSET_REFRESH
    GpuDeferredDeletionQueue& m_deletionQ;
#endif
};

#endif // TEXTURE_TEXTUREASSET_H
