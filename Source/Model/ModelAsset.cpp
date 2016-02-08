#include "Model/ModelAsset.h"

#include <string.h>

#include "Core/Macros.h"
#include "Core/Endian.h"
#include "Core/FileLoader.h"

#include "Asset/AssetCache.h"

#include "Texture/TextureAsset.h"

struct ModelHeader {
    char code[4];
    u32 version;
    u32 nVertices;
    u32 ofsVertices;
    u32 nIndices;
    u32 ofsIndices;
    u32 nTextures;
    u32 ofsTextures;
    u32 diffuseTextureIndex;

    void FixEndian()
    {
        version = EndianSwapLE32(version);
        nVertices = EndianSwapLE32(nVertices);
        ofsVertices = EndianSwapLE32(ofsVertices);
        nIndices = EndianSwapLE32(nIndices);
        ofsIndices = EndianSwapLE32(ofsIndices);
        nTextures = EndianSwapLE32(nTextures);
        ofsTextures = EndianSwapLE32(ofsTextures);
        diffuseTextureIndex = EndianSwapLE32(diffuseTextureIndex);
    }
};

struct ModelTextureInfo {
    u32 type;
    u32 flags;
    u32 lenFilename;
    u32 ofsFilename;

    void FixEndian()
    {
        type = EndianSwapLE32(type);
        flags = EndianSwapLE32(flags);
        lenFilename = EndianSwapLE32(lenFilename);
        ofsFilename = EndianSwapLE32(ofsFilename);
    }
};

void ModelAsset::Vertex::FixEndian()
{
    for (int i = 0; i < 3; ++i) {
        position[i] = EndianSwapLEFloat32(position[i]);
        normal[i] = EndianSwapLEFloat32(normal[i]);
    }
    uv[0] = EndianSwapLEFloat32(uv[0]);
    uv[1] = EndianSwapLEFloat32(uv[1]);
}

ModelAsset::ModelAsset(GpuDevice* device,
                       AssetCache<TextureAsset>& textureCache,
                       u8* data,
                       u32 size)
    : m_device(device)
    , m_nIndices(0)
    , m_vertexBuf(0)
    , m_indexBuf(0)
    , m_diffuseTex(NULL)
{
    ASSERT(device);

    ModelHeader* header = (ModelHeader*)data;
    if (memcmp(header->code, "MODL", 4) != 0)
        FATAL("Model has incorrect header code");
    header->FixEndian();

    u32 nVertices = header->nVertices;
    u32 nIndices = header->nIndices;
    u32 nTextures = header->nTextures;

    Vertex* vertices = (Vertex*)(data + header->ofsVertices);
    for (u32 i = 0; i < nVertices; ++i) {
        vertices[i].FixEndian();
    }
    m_vertexBuf = device->BufferCreate(
        GPU_BUFFER_TYPE_VERTEX,
        GPU_BUFFER_ACCESS_STATIC,
        vertices,
        nVertices * sizeof(Vertex)
    );

    u32* indices = (u32*)(data + header->ofsIndices);
    for (u32 i = 0; i < nIndices; ++i) {
        indices[i] = EndianSwapLE32(indices[i]);
    }
    m_indexBuf = device->BufferCreate(
        GPU_BUFFER_TYPE_INDEX,
        GPU_BUFFER_ACCESS_STATIC,
        indices,
        nIndices * sizeof(u32)
    );

    ModelTextureInfo* textures = (ModelTextureInfo*)(data + header->ofsTextures);
    for (u32 i = 0; i < nTextures; ++i) {
        textures[i].FixEndian();
    }
    if (header->diffuseTextureIndex != 0xFFFFFFFF) {
        const ModelTextureInfo& texInfo = textures[header->diffuseTextureIndex];
        const char* path = (const char*)(data + texInfo.ofsFilename);

        m_diffuseTex = textureCache.FindOrLoad(path);
        m_diffuseTex->AddRef();
    }

    m_nIndices = nIndices;
}

ModelAsset::~ModelAsset()
{
    if (m_diffuseTex)
        m_diffuseTex->Release();
    m_device->BufferDestroy(m_vertexBuf);
    m_device->BufferDestroy(m_indexBuf);
}

GpuDevice* ModelAsset::GetGpuDevice() const
{
    return m_device;
}

u32 ModelAsset::GetIndexCount() const
{
    return m_nIndices;
}

GpuBufferID ModelAsset::GetVertexBuf() const
{
    return m_vertexBuf;
}

GpuBufferID ModelAsset::GetIndexBuf() const
{
    return m_indexBuf;
}

TextureAsset* ModelAsset::GetDiffuseTex() const
{
    return m_diffuseTex;
}

ModelAssetFactory::ModelAssetFactory(GpuDevice* device,
                                     AssetCache<TextureAsset>& textureCache)
    : m_device(device)
    , m_textureCache(textureCache)
{}

void* Alloc(u32 size, void* userdata)
{
    return malloc(size);
}

ModelAsset* ModelAssetFactory::Create(const char* path, FileLoader& loader)
{
    u8* data;
    u32 size;
    loader.Load(path, &data, &size, Alloc, NULL);
    ModelAsset* asset = new ModelAsset(m_device, m_textureCache, data, size);
    free(data);
    return asset;
}

#ifdef ASSET_REFRESH
void ModelAssetFactory::Refresh(ModelAsset* asset, const char* path, FileLoader& loader)
{
    ASSERT(!"ModelAssetFactory::Refresh() not implemented");
}
#endif
