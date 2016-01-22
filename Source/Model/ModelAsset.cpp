#include "Model/ModelAsset.h"
#include <string.h>
#include "Core/Macros.h"
#include "Core/Endian.h"

struct ModelHeader {
    char code[4];
    u32 version;
    u32 nVertices;
    u32 nIndices;
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

ModelAsset::ModelAsset(GpuDevice* device, u8* data, int size)
    : m_device(device)
    , m_nIndices(0)
    , m_vertexBuf(0)
    , m_indexBuf(0)
{
    ASSERT(device);

    ModelHeader* header = (ModelHeader*)data;
    if (memcmp(header->code, "MODL", 4) != 0)
        FATAL("Model has incorrect header code");

    u32 nVertices = EndianSwapLE32(header->nVertices);
    u32 nIndices = EndianSwapLE32(header->nIndices);

    Vertex* vertices = (Vertex*)(data + sizeof(ModelHeader));
    for (u32 i = 0; i < nVertices; ++i) {
        vertices[i].FixEndian();
    }
    m_vertexBuf = device->BufferCreate(
        GPU_BUFFER_TYPE_VERTEX,
        GPU_BUFFER_ACCESS_STATIC,
        vertices,
        nVertices * sizeof(Vertex)
    );

    u32* indices = (u32*)(vertices + nVertices);
    for (u32 i = 0; i < nIndices; ++i) {
        indices[i] = EndianSwapLE32(indices[i]);
    }
    m_indexBuf = device->BufferCreate(
        GPU_BUFFER_TYPE_INDEX,
        GPU_BUFFER_ACCESS_STATIC,
        indices,
        nIndices * sizeof(u32)
    );

    m_nIndices = nIndices;
}

ModelAsset::~ModelAsset()
{
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

ModelAssetFactory::ModelAssetFactory(GpuDevice* device)
    : m_device(device)
{}

ModelAsset* ModelAssetFactory::operator()(u8* data, int size) const
{
    return new ModelAsset(m_device, data, size);
}
