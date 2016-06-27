#ifndef MODEL_MODELSHARED_H
#define MODEL_MODELSHARED_H

#include "Core/Types.h"
#include "GpuDevice/GpuDevice.h"

class FileLoader;
class TextureAsset;
class TextureCache;
class ModelInstance;

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

class ModelShared {
public:
    struct Vertex {
        float position[3];
        float normal[3];
        float uv[2];
    };

    static ModelShared* Create(
        GpuDevice& device,
        TextureCache& textureCache,
        FileLoader& loader,
        const char* path
    );
    static void Destroy(ModelShared* shared);

    int RefCount() const;
    void AddRef();
    void Release();

    const u8* GetMDLData() const;
    GpuDevice& GetGpuDevice() const;
    GpuBufferID GetVertexBuf() const;
    GpuBufferID GetIndexBuf() const;

    void SetFirstInstance(ModelInstance* instance);
    ModelInstance* GetFirstInstance() const;

private:
    ModelShared(const ModelShared&);
    ModelShared& operator=(const ModelShared&);

    ModelShared(
        GpuDevice& device,
        TextureCache& textureCache,
        u8* mdlData,
        u8* mdgData
    );
    ~ModelShared();

    GpuDevice& m_device;
    GpuBufferID m_vertexBuf;
    GpuBufferID m_indexBuf;
    ModelInstance* m_firstInstance;
    int m_refCount;
};

#endif // MODEL_MODELSHARED_H
