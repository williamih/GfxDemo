#ifndef MODEL_MODELASSET_H
#define MODEL_MODELASSET_H

#include "Core/Types.h"
#include "GpuDevice/GpuDevice.h"
#include "Asset/Asset.h"

template<class T> class AssetCache;
class TextureAsset;

struct MDLHeader {
    char code[4];
    u32 version;
    u32 nSubmeshes;
    u32 ofsSubmeshes;
};

struct MDLSubmesh {
    u32 indexStart;
    u32 indexCount;
    union {
        u64 diffuseTextureIndex;
        TextureAsset* diffuseTexture;
    };
};

class ModelAsset : public Asset {
public:
    struct Vertex {
        float position[3];
        float normal[3];
        float uv[2];
    };

    static ModelAsset* Create(
        GpuDevice& device,
        AssetCache<TextureAsset>& textureCache,
        FileLoader& loader,
        const char* path
    );

    const u8* GetMDLData() const;
    GpuDevice& GetGpuDevice() const;
    GpuBufferID GetVertexBuf() const;
    GpuBufferID GetIndexBuf() const;

private:
    ModelAsset(const ModelAsset&);
    ModelAsset& operator=(const ModelAsset&);

    ModelAsset(
        GpuDevice& device,
        AssetCache<TextureAsset>& textureCache,
        u8* mdlData,
        u8* mdgData
    );
    virtual ~ModelAsset();

    virtual void Destroy();

    GpuDevice& m_device;
    GpuBufferID m_vertexBuf;
    GpuBufferID m_indexBuf;
#ifdef ASSET_REFRESH
    const u8* m_data;
#endif
};

class ModelAssetFactory : public AssetFactory<ModelAsset> {
public:
    ModelAssetFactory(GpuDevice& device, AssetCache<TextureAsset>& textureCache);

    virtual ModelAsset* Create(const char* path, FileLoader& loader);

#ifdef ASSET_REFRESH
    virtual void Refresh(ModelAsset* asset, const char* path, FileLoader& loader);
#endif
private:
    GpuDevice& m_device;
    AssetCache<TextureAsset>& m_textureCache;
};

#endif // MODEL_MODELASSET_H
