#ifndef MODEL_MODELASSET_H
#define MODEL_MODELASSET_H

#include "Core/Types.h"
#include "GpuDevice/GpuDevice.h"

class ModelAsset {
public:
    struct Vertex {
        float position[3];
        float normal[3];
        float uv[2];

        void FixEndian();
    };

    ModelAsset(GpuDevice* device, u8* data, int size);
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

class ModelAssetFactory {
public:
    explicit ModelAssetFactory(GpuDevice* device);

    ModelAsset* operator()(u8* data, int size) const;
private:
    GpuDevice* m_device;
};

#endif // MODEL_MODELASSET_H
