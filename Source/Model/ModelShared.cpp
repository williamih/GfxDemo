#include "Model/ModelShared.h"

#include <string.h>

#include "Core/Macros.h"
#include "Core/Endian.h"
#include "Core/FileLoader.h"
#include "Core/Path.h"

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

static void FixEndian(ModelShared::Vertex& v)
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

    ModelShared::Vertex* vertices = (ModelShared::Vertex*)(mdgData + mdgHeader->ofsVertices);
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

ModelShared::ModelShared(
    GpuDevice& device,
    TextureCache& textureCache,
    u8* mdlData,
    u8* mdgData,
    const char* path
)
    : m_link()

    , m_device(device)
    , m_vertexBuf(0)
    , m_indexBuf(0)
    , m_firstInstance(NULL)
    , m_refCount(0)
    , m_path()
{
    StrCopy(m_path, sizeof m_path, path);

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
        mdgHeader->nVertices * sizeof(Vertex),
        0 // maxUpdatesPerFrame (unused)
    );

    m_indexBuf = device.BufferCreate(
        GPU_BUFFER_TYPE_INDEX,
        GPU_BUFFER_ACCESS_STATIC,
        mdgData + mdgHeader->ofsIndices,
        mdgHeader->nIndices * sizeof(u32),
        0 // maxUpdatesPerFrame (unused)
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

ModelShared::~ModelShared()
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
}

void* MDLAlloc(u32 size, void* userdata)
{
    void* memory = malloc(sizeof(ModelShared) + size);
    return (u8*)memory + sizeof(ModelShared);
}

void* MDGAlloc(u32 size, void* userdata)
{
    return malloc(size);
}

ModelShared* ModelShared::Create(
    GpuDevice& device,
    TextureCache& textureCache,
    FileLoader& loader,
    const char* path
)
{
    u8* mdlData;
    loader.Load(path, &mdlData, NULL, MDLAlloc, NULL);

    char mdgPath[MAX_PATH_LENGTH];
    PathReplaceExtension(mdgPath, sizeof mdgPath, path, ".mdg");

    u8* mdgData;
    loader.Load(mdgPath, &mdgData, NULL, MDGAlloc, NULL);

    void* assetLocation = mdlData - sizeof(ModelShared);
    ModelShared* asset = new (assetLocation) ModelShared(
        device,
        textureCache,
        mdlData,
        mdgData,
        path
    );

    free(mdgData);

    return asset;
}

void ModelShared::Destroy(ModelShared* shared)
{
    shared->~ModelShared();
    free((void*)shared);
}

int ModelShared::RefCount() const
{
    return m_refCount;
}

void ModelShared::AddRef()
{
    ++m_refCount;
}

void ModelShared::Release()
{
    ASSERT(m_refCount > 0);
    --m_refCount;
}

const char* ModelShared::GetPath() const
{
    return m_path;
}

const u8* ModelShared::GetMDLData() const
{
    return (const u8*)(this + 1);
}

GpuDevice& ModelShared::GetGpuDevice() const
{
    return m_device;
}

GpuBufferID ModelShared::GetVertexBuf() const
{
    return m_vertexBuf;
}

GpuBufferID ModelShared::GetIndexBuf() const
{
    return m_indexBuf;
}

void ModelShared::SetFirstInstance(ModelInstance* instance)
{
    m_firstInstance = instance;
}

ModelInstance* ModelShared::GetFirstInstance() const
{
    return m_firstInstance;
}
