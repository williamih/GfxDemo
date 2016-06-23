#include "Model/ModelAsset.h"

#include <string.h>

#include "Core/Macros.h"
#include "Core/Endian.h"
#include "Core/FileLoader.h"
#include "Core/Path.h"

#include "Asset/AssetCache.h"

#include "Texture/TextureAsset.h"

struct MDGHeader {
    char code[4];
    u32 nVertices;
    u32 ofsVertices;
    u32 nIndices;
    u32 ofsIndices;
    u32 nTextures;
    u32 ofsTextures;
};

struct MDGTextureInfo {
    u32 lenFilename;
    u32 ofsFilename;
};

static void FixEndian(MDLHeader& s)
{
    s.version = EndianSwapLE32(s.version);
    s.nSubmeshes = EndianSwapLE32(s.nSubmeshes);
    s.ofsSubmeshes = EndianSwapLE32(s.ofsSubmeshes);
}

static void FixEndian(MDLSubmesh& s)
{
    s.indexStart = EndianSwapLE32(s.indexStart);
    s.indexCount = EndianSwapLE32(s.indexCount);
    s.diffuseTextureIndex = EndianSwapLE64(s.diffuseTextureIndex);
}

static void FixEndian(MDGHeader& s)
{
    s.nVertices = EndianSwapLE32(s.nVertices);
    s.ofsVertices = EndianSwapLE32(s.ofsVertices);
    s.nIndices = EndianSwapLE32(s.nIndices);
    s.ofsIndices = EndianSwapLE32(s.ofsIndices);
    s.nTextures = EndianSwapLE32(s.nTextures);
    s.ofsTextures = EndianSwapLE32(s.ofsTextures);
}

static void FixEndian(MDGTextureInfo& s)
{
    s.lenFilename = EndianSwapLE32(s.lenFilename);
    s.ofsFilename = EndianSwapLE32(s.ofsFilename);
}

static void FixEndian(ModelAsset::Vertex& v)
{
    for (int i = 0; i < 3; ++i) {
        v.position[i] = EndianSwapLEFloat32(v.position[i]);
        v.normal[i] = EndianSwapLEFloat32(v.normal[i]);
    }
    v.uv[0] = EndianSwapLEFloat32(v.uv[0]);
    v.uv[1] = EndianSwapLEFloat32(v.uv[1]);
}

static void MDLFixEndian(u8* mdlData)
{
    MDLHeader* mdlHeader = (MDLHeader*)mdlData;
    FixEndian(*mdlHeader);

    u32 nSubmeshes = mdlHeader->nSubmeshes;
    MDLSubmesh* submeshes = (MDLSubmesh*)(mdlData + mdlHeader->ofsSubmeshes);
    for (u32 i = 0; i < nSubmeshes; ++i) {
        FixEndian(submeshes[i]);
    }
}

static void MDGFixEndian(u8* mdgData)
{
    MDGHeader* mdgHeader = (MDGHeader*)mdgData;
    FixEndian(*mdgHeader);

    u32 nVertices = mdgHeader->nVertices;
    u32 nIndices = mdgHeader->nIndices;
    u32 nTextures = mdgHeader->nTextures;

    ModelAsset::Vertex* vertices = (ModelAsset::Vertex*)(mdgData + mdgHeader->ofsVertices);
    for (u32 i = 0; i < nVertices; ++i) {
        FixEndian(vertices[i]);
    }

    u32* indices = (u32*)(mdgData + mdgHeader->ofsIndices);
    for (u32 i = 0; i < nIndices; ++i) {
        indices[i] = EndianSwapLE32(indices[i]);
    }

    MDGTextureInfo* textures = (MDGTextureInfo*)(mdgData + mdgHeader->ofsTextures);
    for (u32 i = 0; i < nTextures; ++i) {
        FixEndian(textures[i]);
    }
}

ModelAsset::ModelAsset(
    GpuDevice& device,
    AssetCache<TextureAsset>& textureCache,
    u8* mdlData,
    u8* mdgData
)
    : m_device(device)
    , m_vertexBuf(0)
    , m_indexBuf(0)
#ifdef ASSET_REFRESH
    , m_data(mdlData)
#endif
{
    MDLHeader* mdlHeader = (MDLHeader*)GetMDLData();
    if (memcmp(mdlHeader->code, "MODL", 4) != 0)
        FATAL("Model has incorrect header code");

    MDLFixEndian((u8*)GetMDLData());
    MDGFixEndian(mdgData);

    MDGHeader* mdgHeader = (MDGHeader*)mdgData;

    m_vertexBuf = device.BufferCreate(
        GPU_BUFFER_TYPE_VERTEX,
        GPU_BUFFER_ACCESS_STATIC,
        mdgData + mdgHeader->ofsVertices,
        mdgHeader->nVertices * sizeof(Vertex)
    );

    m_indexBuf = device.BufferCreate(
        GPU_BUFFER_TYPE_INDEX,
        GPU_BUFFER_ACCESS_STATIC,
        mdgData + mdgHeader->ofsIndices,
        mdgHeader->nIndices * sizeof(u32)
    );

    MDGTextureInfo* textures = (MDGTextureInfo*)(mdgData + mdgHeader->ofsTextures);

    u32 nSubmeshes = mdlHeader->nSubmeshes;
    MDLSubmesh* submeshes = (MDLSubmesh*)(GetMDLData() + mdlHeader->ofsSubmeshes);
    for (u32 i = 0; i < nSubmeshes; ++i) {
        if (submeshes[i].diffuseTextureIndex == 0xFFFFFFFFFFFFFFFF) {
            submeshes[i].diffuseTexture = NULL;
        } else {
            MDGTextureInfo& textureInfo = textures[submeshes[i].diffuseTextureIndex];
            const char* filename = (const char*)(mdgData + textureInfo.ofsFilename);

            TextureAsset* texture = textureCache.FindOrLoad(filename);
            texture->AddRef();

            submeshes[i].diffuseTexture = texture;
        }
    }
}

ModelAsset::~ModelAsset()
{
    MDLHeader* mdlHeader = (MDLHeader*)GetMDLData();

    // Release all the textures
    u32 nSubmeshes = mdlHeader->nSubmeshes;
    MDLSubmesh* submeshes = (MDLSubmesh*)(GetMDLData() + mdlHeader->ofsSubmeshes);
    for (u32 i = 0; i < nSubmeshes; ++i) {
        if (submeshes[i].diffuseTexture)
            submeshes[i].diffuseTexture->Release();
    }

    m_device.BufferDestroy(m_vertexBuf);
    m_device.BufferDestroy(m_indexBuf);

#ifdef ASSET_REFRESH
    free((void*)m_data);
#endif
}

void ModelAsset::Destroy()
{
#ifdef ASSET_REFRESH
    delete this;
#else
    this->~ModelAsset();
    free((void*)this);
#endif
}

void* MDLAlloc(u32 size, void* userdata)
{
#ifdef ASSET_REFRESH
    return malloc(size);
#else
    void* memory = malloc(sizeof(ModelAsset) + size);
    return (u8*)memory + sizeof(ModelAsset);
#endif
}

void* MDGAlloc(u32 size, void* userdata)
{
    return malloc(size);
}

ModelAsset* ModelAsset::Create(
    GpuDevice& device,
    AssetCache<TextureAsset>& textureCache,
    FileLoader& loader,
    const char* path
)
{
    u8* mdlData;
    loader.Load(path, &mdlData, NULL, MDLAlloc, NULL);

    char mdgPath[260];
    PathReplaceExtension(mdgPath, sizeof mdgPath, path, ".mdg");

    u8* mdgData;
    loader.Load(mdgPath, &mdgData, NULL, MDGAlloc, NULL);

#ifdef ASSET_REFRESH
    ModelAsset* asset = new ModelAsset(
        device,
        textureCache,
        mdlData,
        mdgData
    );
#else
    void* assetLocation = mdlData - sizeof(ModelAsset);
    ModelAsset* asset = new (assetLocation) ModelAsset(
        device,
        textureCache,
        mdlData,
        mdgData
    );
#endif

    free(mdgData);

    return asset;
}


const u8* ModelAsset::GetMDLData() const
{
#ifdef ASSET_REFRESH
    return m_data;
#else
    return (const u8*)(this + 1);
#endif
}

GpuDevice& ModelAsset::GetGpuDevice() const
{
    return m_device;
}

GpuBufferID ModelAsset::GetVertexBuf() const
{
    return m_vertexBuf;
}

GpuBufferID ModelAsset::GetIndexBuf() const
{
    return m_indexBuf;
}

ModelAssetFactory::ModelAssetFactory(GpuDevice& device,
                                     AssetCache<TextureAsset>& textureCache)
    : m_device(device)
    , m_textureCache(textureCache)
{}

ModelAsset* ModelAssetFactory::Create(const char* path, FileLoader& loader)
{
    return ModelAsset::Create(m_device, m_textureCache, loader, path);
}

#ifdef ASSET_REFRESH
void ModelAssetFactory::Refresh(ModelAsset* asset, const char* path, FileLoader& loader)
{
    ASSERT(!"ModelAssetFactory::Refresh() not implemented");
}
#endif
