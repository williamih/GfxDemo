#ifndef MODEL_MODELASSET_H
#define MODEL_MODELASSET_H

#include "Core/Types.h"
#include "GpuDevice/GpuDevice.h"
#include "Asset/Asset.h"

class ModelAsset {
public:
    struct Vertex {
        float position[3];
        float normal[3];
        float uv[2];

        void FixEndian();
    };

    ModelAsset(GpuDevice* device, u8* data, u32 size);
    ~ModelAsset();

    GpuDevice* GetGpuDevice() const;
    u32 GetIndexCount() const;
    GpuBufferID GetVertexBuf() const;
    GpuBufferID GetIndexBuf() const;
private:
    ModelAsset(const ModelAsset&);
    ModelAsset& operator=(const ModelAsset&);

    GpuDevice* m_device;
    u32 m_nIndices;
    GpuBufferID m_vertexBuf;
    GpuBufferID m_indexBuf;
};

class ModelAssetFactory : public AssetFactory<ModelAsset> {
public:
    explicit ModelAssetFactory(GpuDevice* device);

    virtual void* Allocate(u32 size);

    virtual ModelAsset* Create(u8* data, u32 size, const char* path);

#ifdef ASSET_REFRESH
    virtual void Refresh(ModelAsset* asset, u8* data, u32 size, const char* path);
#endif
private:
    GpuDevice* m_device;
};

#endif // MODEL_MODELASSET_H
