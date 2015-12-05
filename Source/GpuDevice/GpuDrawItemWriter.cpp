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
{}

void GpuDrawItemWriterDesc::SetNumCBuffers(int n)
{
    ASSERT(n > 0 && n <= 255);
    m_nCBuffers = n;
}

void GpuDrawItemWriterDesc::SetNumVertexBuffers(int n)
{
    ASSERT(n > 0 && n <= 255);
    m_nVertexBuffers = n;
}

size_t GpuDrawItemWriter::SizeInBytes(const GpuDrawItemWriterDesc& desc)
{
    size_t size = sizeof(GpuDrawItem);
    size += desc.m_nVertexBuffers * sizeof(u32);
    size += desc.m_nVertexBuffers * sizeof(u16);
    size += desc.m_nCBuffers * sizeof(u16);
    return size;
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
    ASSERT(desc.m_nVertexBuffers > 0 && desc.m_nVertexBuffers <= 255);
    ASSERT(desc.m_nCBuffers > 0 && desc.m_nCBuffers <= 255);

    m_device = device;

    size_t size = SizeInBytes(desc);
    m_drawItem = (GpuDrawItem*)alloc(size, userdata);
    m_desc = desc;
    m_flags = 0;

    m_drawItem->pipelineStateIdx = 0xFFFF;
    m_drawItem->flags = 0;
    m_drawItem->indexBufferIdx = 0xFFFF;
    m_drawItem->nVertexBuffers = (u8)desc.m_nVertexBuffers;
    m_drawItem->nCBuffers = (u8)desc.m_nCBuffers;
    m_drawItem->first = 0;
    m_drawItem->count = 0;
    m_drawItem->indexBufferOffset = 0;
    memset(m_drawItem->VertexBuffers(), 0xFF, desc.m_nVertexBuffers * sizeof(u16));
    memset(m_drawItem->CBuffers(), 0xFF, desc.m_nCBuffers * sizeof(u16));
}

GpuDrawItem* GpuDrawItemWriter::End()
{
    ASSERT(m_drawItem != NULL && "Must call Begin() before End()");

    // Validation code
    ValidateVertexBuffers();
    ValidateCBuffers();
    ASSERT(m_drawItem->pipelineStateIdx != 0xFFFF
           && "No pipeline state object has been specified");
    ASSERT((m_flags & FLAG_SETDRAWCALL) && "No draw call has been specified");
    ASSERT(!(m_flags & FLAG_INDEXED) || (m_drawItem->indexBufferIdx != 0xFFFF)
           && "Index buffer not specified for indexed draw call");

    m_device->RegisterDrawItem(m_drawItem);
    m_device = NULL;

    GpuDrawItem* item = m_drawItem;
    m_drawItem = NULL;
    return item;
}

void GpuDrawItemWriter::ValidateVertexBuffers()
{
    u16* bufs = m_drawItem->VertexBuffers();
    for (int i = 0; i < m_desc.m_nVertexBuffers; ++i) {
        if (bufs[i] == 0xFFFF)
            FATAL("GpuDrawItemWriter: vertex buffer (index %d) not set", i);
    }
}

void GpuDrawItemWriter::ValidateCBuffers()
{
    u16* cbufs = m_drawItem->CBuffers();
    for (int i = 0; i < m_desc.m_nVertexBuffers; ++i) {
        if (cbufs[i] == 0xFFFF)
            FATAL("GpuDrawItemWriter: constant buffer (index %d) not set", i);
    }
}

void GpuDrawItemWriter::SetPipelineState(GpuPipelineStateID state)
{
    ASSERT(m_device->PipelineStateObjectIDExists(state));
    m_drawItem->pipelineStateIdx = GetRawIndex(state);
}

void GpuDrawItemWriter::SetIndexBuffer(GpuBufferID buffer)
{
    ASSERT(m_device->BufferIDExists(buffer));
    m_drawItem->indexBufferIdx = GetRawIndex(buffer);
}

void GpuDrawItemWriter::SetVertexBuffer(int index, GpuBufferID buffer, unsigned offset)
{
    ASSERT(index >= 0 && index < m_desc.m_nVertexBuffers);
    ASSERT(m_device->BufferIDExists(buffer));
    m_drawItem->VertexBufferOffsets()[index] = (u32)offset;
    m_drawItem->VertexBuffers()[index] = GetRawIndex(buffer);
}

void GpuDrawItemWriter::SetCBuffer(int index, GpuBufferID buffer)
{
    ASSERT(index >= 0 && index < m_desc.m_nCBuffers);
    ASSERT(m_device->BufferIDExists(buffer));
    m_drawItem->CBuffers()[index] = GetRawIndex(buffer);
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
