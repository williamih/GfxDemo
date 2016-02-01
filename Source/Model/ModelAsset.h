#ifndef MODEL_MODELASSET_H
#define MODEL_MODELASSET_H

#include <memory>

#include "Core/Types.h"
#include "GpuDevice/GpuDevice.h"
#include "Asset/Asset.h"

template<class T> class AssetCache;
class TextureAsset;

class ModelAsset {
public:
    struct Vertex {
        float position[3];
        float normal[3];
        float uv[2];

        void FixEndian();
    };

    ModelAsset(GpuDevice* device,
               AssetCache<TextureAsset>& textureCache,
               u8* data,
               u32 size);
    ~ModelAsset();

    GpuDevice* GetGpuDevice() const;
    u32 GetIndexCount() const;
    GpuBufferID GetVertexBuf() const;
    GpuBufferID GetIndexBuf() const;

    // May return NULL
    TextureAsset* GetDiffuseTex() const;
private:
    ModelAsset(const ModelAsset&);
    ModelAsset& operator=(const ModelAsset&);

    GpuDevice* m_device;
    u32 m_nIndices;
    GpuBufferID m_vertexBuf;
    GpuBufferID m_indexBuf;
    std::shared_ptr<TextureAsset> m_diffuseTex;
};

class ModelAssetFactory : public AssetFactory<ModelAsset> {
public:
    ModelAssetFactory(GpuDevice* device, AssetCache<TextureAsset>& textureCache);

    virtual std::shared_ptr<ModelAsset> Create(const char* path, FileLoader& loader);

#ifdef ASSET_REFRESH
    virtual void Refresh(ModelAsset* asset, const char* path, FileLoader& loader);
#endif
private:
    GpuDevice* m_device;
    AssetCache<TextureAsset>& m_textureCache;
};

#endif // MODEL_MODELASSET_H
