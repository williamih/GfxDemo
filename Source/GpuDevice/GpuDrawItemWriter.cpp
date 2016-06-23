#include "GpuDevice/GpuDrawItemWriter.h"
#include <string.h>
#include "Core/Macros.h"
#include "GpuDevice/GpuDrawItem.h"

const u32 FLAG_SETDRAWCALL = 1;
const u32 FLAG_INDEXED = 2;

static u16 GetRawIndex(u32 resourceID)
{
    return (u16)(resourceID & 0xFFFF);
}

GpuDrawItemWriterDesc::GpuDrawItemWriterDesc()
    : m_nCBuffers(0)
    , m_nVertexBuffers(0)
    , m_nTextures(0)
    , m_nSamplers(0)
{}

void GpuDrawItemWriterDesc::SetNumCBuffers(int n)
{
    ASSERT(n >= 0 && n <= 255);
    m_nCBuffers = n;
}

void GpuDrawItemWriterDesc::SetNumVertexBuffers(int n)
{
    ASSERT(n > 0 && n <= 255);
    m_nVertexBuffers = n;
}

void GpuDrawItemWriterDesc::SetNumTextures(int n)
{
    ASSERT(n > 0 && n <= 255);
    m_nTextures = n;
}

void GpuDrawItemWriterDesc::SetNumSamplers(int n)
{
    ASSERT(n > 0 && n <= 255);
    m_nSamplers = n;
}

size_t GpuDrawItemWriter::SizeInBytes(const GpuDrawItemWriterDesc& desc)
{
    size_t size = sizeof(GpuDrawItem);
    size += desc.NumVertexBuffers() * sizeof(u32);
    size += desc.NumVertexBuffers() * sizeof(u16);
    size += desc.NumCBuffers() * sizeof(u16);
    size += desc.NumTextures() * sizeof(u16);
    size += desc.NumSamplers() * sizeof(u16);
    return size;
}

static void ValidateVertexBuffers(const GpuDrawItem& item,
                                  const GpuDrawItemWriterDesc& desc)
{
    const u16* bufs = item.VertexBuffers();
    for (int i = 0; i < desc.NumVertexBuffers(); ++i) {
        if (bufs[i] == 0xFFFF)
            FATAL("GpuDrawItemWriter: vertex buffer (index %d) not set", i);
    }
}

static void ValidateCBuffers(const GpuDrawItem& item,
                             const GpuDrawItemWriterDesc& desc)
{
    const u16* cbufs = item.CBuffers();
    for (int i = 0; i < desc.NumCBuffers(); ++i) {
        if (cbufs[i] == 0xFFFF)
            FATAL("GpuDrawItemWriter: constant buffer (index %d) not set", i);
    }
}

static void ValidateTextures(const GpuDrawItem& item,
                             const GpuDrawItemWriterDesc& desc)
{
    const u16* textures = item.Textures();
    for (int i = 0; i < desc.NumTextures(); ++i) {
        if (textures[i] == 0xFFFF)
            FATAL("GpuDrawItemWriter: texture (index %d) not set", i);
    }
}

static void ValidateSamplers(const GpuDrawItem& item,
                             const GpuDrawItemWriterDesc& desc)
{
    const u16* samplers = item.Samplers();
    for (int i = 0; i < desc.NumSamplers(); ++i) {
        if (samplers[i] == 0xFFFF)
            FATAL("GpuDrawItemWriter: sampler (index %d) not set", i);
    }
}

GpuDrawItemWriter::GpuDrawItemWriter()
    : m_device(NULL)
    , m_drawItem(NULL)
    , m_desc()
    , m_flags(0)
{}

void GpuDrawItemWriter::Begin(GpuDevice* device,
                              const GpuDrawItemWriterDesc& desc,
                              PFnAlloc alloc,
                              void* userdata)
{
    ASSERT(device != NULL);
    ASSERT(!m_drawItem && "Must call End() before calling Begin() again");
    ASSERT(desc.NumVertexBuffers() > 0 && desc.NumVertexBuffers() <= 255);
    ASSERT(desc.NumCBuffers() >= 0 && desc.NumCBuffers() <= 255);
    ASSERT(desc.NumTextures() >= 0 && desc.NumTextures() <= 255);
    ASSERT(desc.NumSamplers() >= 0 && desc.NumSamplers() <= 255);

    m_device = device;

    size_t size = SizeInBytes(desc);
    m_drawItem = (GpuDrawItem*)alloc(size, userdata);
    m_desc = desc;
    m_flags = 0;

    m_drawItem->pipelineStateIdx = 0xFFFF;
    m_drawItem->flags = 0;
    m_drawItem->indexBufferIdx = 0xFFFF;
    m_drawItem->nVertexBuffers = (u8)desc.NumVertexBuffers();
    m_drawItem->nCBuffers = (u8)desc.NumCBuffers();
    m_drawItem->nTextures = (u8)desc.NumTextures();
    m_drawItem->nSamplers = (u8)desc.NumSamplers();
    m_drawItem->_pad = 0;
    m_drawItem->first = 0;
    m_drawItem->count = 0;
    m_drawItem->indexBufferOffset = 0;

    memset(m_drawItem->VertexBuffers(), 0xFF, desc.NumVertexBuffers() * sizeof(u16));
    memset(m_drawItem->CBuffers(), 0xFF, desc.NumCBuffers() * sizeof(u16));
    memset(m_drawItem->Textures(), 0xFF, desc.NumTextures() * sizeof(u16));
    memset(m_drawItem->Samplers(), 0xFF, desc.NumSamplers() * sizeof(u16));
}

GpuDrawItem* GpuDrawItemWriter::End()
{
    ASSERT(m_drawItem != NULL && "Must call Begin() before End()");

    // Validation code
    ValidateVertexBuffers(*m_drawItem, m_desc);
    ValidateCBuffers(*m_drawItem, m_desc);
    ValidateTextures(*m_drawItem, m_desc);
    ValidateSamplers(*m_drawItem, m_desc);
    ASSERT(m_drawItem->pipelineStateIdx != 0xFFFF
           && "No pipeline state object has been specified");
    ASSERT((m_flags & FLAG_SETDRAWCALL) && "No draw call has been specified");
    ASSERT(!(m_flags & FLAG_INDEXED) || (m_drawItem->indexBufferIdx != 0xFFFF)
           && "Index buffer not specified for indexed draw call");

#ifdef GPUDEVICE_DEBUG_MODE
    m_device->RegisterDrawItem(m_drawItem);
#endif
    m_device = NULL;

    GpuDrawItem* item = m_drawItem;
    m_drawItem = NULL;
    return item;
}

void GpuDrawItemWriter::SetPipelineState(GpuPipelineStateID state)
{
    ASSERT(m_device->PipelineStateExists(state));
    m_drawItem->pipelineStateIdx = GetRawIndex(state);
}

void GpuDrawItemWriter::SetIndexBuffer(GpuBufferID buffer)
{
    ASSERT(m_device->BufferExists(buffer));
    m_drawItem->indexBufferIdx = GetRawIndex(buffer);
}

void GpuDrawItemWriter::SetVertexBuffer(int index, GpuBufferID buffer, unsigned offset)
{
    ASSERT(index >= 0 && index < m_desc.NumVertexBuffers());
    ASSERT(m_device->BufferExists(buffer));
    m_drawItem->VertexBufferOffsets()[index] = (u32)offset;
    m_drawItem->VertexBuffers()[index] = GetRawIndex(buffer);
}

void GpuDrawItemWriter::SetCBuffer(int index, GpuBufferID buffer)
{
    ASSERT(index >= 0 && index < m_desc.NumCBuffers());
    ASSERT(m_device->BufferExists(buffer));
    m_drawItem->CBuffers()[index] = GetRawIndex(buffer);
}

void GpuDrawItemWriter::SetTexture(int index, GpuTextureID texture)
{
    ASSERT(index >= 0 && index < m_desc.NumTextures());
    ASSERT(m_device->TextureExists(texture));
    m_drawItem->Textures()[index] = GetRawIndex(texture);
}

void GpuDrawItemWriter::SetSampler(int index, GpuSamplerID sampler)
{
    ASSERT(index >= 0 && index < m_desc.NumSamplers());
    ASSERT(m_device->SamplerExists(sampler));
    m_drawItem->Samplers()[index] = GetRawIndex(sampler);
}

void GpuDrawItemWriter::SetDrawCall(GpuPrimitiveType primType, int first, int count)
{
    ASSERT(first >= 0 && count >= 0);
    ASSERT(!(m_flags & FLAG_SETDRAWCALL) && "Draw call already set");

    m_drawItem->SetPrimitiveType(primType);
    m_drawItem->first = (u32)first;
    m_drawItem->count = (u32)count;

    m_flags |= FLAG_SETDRAWCALL;
}

void GpuDrawItemWriter::SetDrawCallIndexed(GpuPrimitiveType primType,
                                           int first,
                                           int count,
                                           unsigned indexBufOffset,
                                           GpuIndexType indexType)
{
    ASSERT(first >= 0 && count >= 0);
    ASSERT(!(m_flags & FLAG_SETDRAWCALL) && "Draw call already set");

    m_drawItem->SetPrimitiveType(primType);
    m_drawItem->SetIndexType(indexType);
    m_drawItem->EnableIndexed();
    m_drawItem->first = (u32)first;
    m_drawItem->count = (u32)count;
    m_drawItem->indexBufferOffset = indexBufOffset;

    m_flags |= FLAG_SETDRAWCALL;
    m_flags |= FLAG_INDEXED;
}
