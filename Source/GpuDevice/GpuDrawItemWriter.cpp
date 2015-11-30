#include "GpuDevice/GpuDrawItemWriter.h"
#include <string.h>
#include "Core/Macros.h"
#include "GpuDevice/GpuDrawItem.h"

const u32 FLAG_SETDRAWCALL = 1;
const u32 FLAG_INDEXED = 2;
const u32 FLAG_SETINDEXBUFFER = 4;
const u32 FLAG_SETPIPELINESTATE = 8;

GpuDrawItemWriterDesc::GpuDrawItemWriterDesc()
    : m_nCBuffers(0)
    , m_nVertexBuffers(0)
{}

void GpuDrawItemWriterDesc::SetNumCBuffers(int n)
{
    m_nCBuffers = n;
}

void GpuDrawItemWriterDesc::SetNumVertexBuffers(int n)
{
    m_nVertexBuffers = n;
}

size_t GpuDrawItemWriter::SizeInBytes(const GpuDrawItemWriterDesc& desc)
{
    size_t size = sizeof(GpuDrawItem);
    size += desc.m_nVertexBuffers * sizeof(GpuDrawItem::VertexBufEntry);
    size += desc.m_nCBuffers * sizeof(u32);
    return size;
}

GpuDrawItemWriter::GpuDrawItemWriter()
    : m_drawItem(NULL)
    , m_desc()
    , m_flags(0)
{}

void GpuDrawItemWriter::Begin(const GpuDrawItemWriterDesc& desc,
                              PFnAlloc alloc,
                              void* userdata)
{
    ASSERT(!m_drawItem && "Must call End() before calling Begin() again");

    size_t size = SizeInBytes(desc);
    m_drawItem = (GpuDrawItem*)alloc(size, userdata);
    m_desc = desc;
    m_flags = 0;

    memset(m_drawItem, 0, size);
    m_drawItem->nVertexBuffers = desc.m_nVertexBuffers;
    m_drawItem->nCBuffers = desc.m_nCBuffers;
}

GpuDrawItem* GpuDrawItemWriter::End()
{
    ASSERT(m_drawItem != NULL && "Must call Begin() before End()");

    // Validation code
    ValidateVertexBuffers();
    ValidateCBuffers();
    ASSERT((m_flags & FLAG_SETPIPELINESTATE)
           && "No pipeline state object has been specified");
    ASSERT((m_flags & FLAG_SETDRAWCALL) && "No draw call has been specified");
    ASSERT(!(m_flags & FLAG_INDEXED) || (m_flags & FLAG_SETINDEXBUFFER)
           && "Index buffer not specified for indexed draw call");

    GpuDrawItem* item = m_drawItem;
    m_drawItem = NULL;
    return item;
}

void GpuDrawItemWriter::ValidateVertexBuffers()
{
    GpuDrawItem::VertexBufEntry* bufs = m_drawItem->VertexBuffers();
    for (int i = 0; i < m_desc.m_nVertexBuffers; ++i) {
        if (bufs[i].bufferID == 0)
            FATAL("GpuDrawItemWriter: vertex buffer (index %d) not set", i);
    }
}

void GpuDrawItemWriter::ValidateCBuffers()
{
    u32* cbufs = m_drawItem->CBuffers();
    for (int i = 0; i < m_desc.m_nVertexBuffers; ++i) {
        if (cbufs[i] == 0)
            FATAL("GpuDrawItemWriter: constant buffer (index %d) not set", i);
    }
}

void GpuDrawItemWriter::SetPipelineState(GpuPipelineStateID state)
{
    m_drawItem->pipelineStateID = state;

    m_flags |= FLAG_SETPIPELINESTATE;
}

void GpuDrawItemWriter::SetIndexBuffer(GpuBufferID buffer)
{
    m_drawItem->indexBufferID = buffer;

    m_flags |= FLAG_SETINDEXBUFFER;
}

void GpuDrawItemWriter::SetVertexBuffer(int index, GpuBufferID buffer, unsigned offset)
{
    ASSERT(index >= 0 && index < m_desc.m_nVertexBuffers);
    ASSERT(buffer != 0);
    GpuDrawItem::VertexBufEntry* bufs = m_drawItem->VertexBuffers();
    bufs[index].bufferID = buffer;
    bufs[index].offset = offset;
}

void GpuDrawItemWriter::SetCBuffer(int index, GpuBufferID buffer)
{
    ASSERT(index >= 0 && index < m_desc.m_nCBuffers);
    ASSERT(buffer != 0);
    u32* cbufs = m_drawItem->CBuffers();
    cbufs[index] = buffer;
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
