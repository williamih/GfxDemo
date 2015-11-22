#ifndef GPUDEVICE_GPUDRAWITEMWRITER_H
#define GPUDEVICE_GPUDRAWITEMWRITER_H

#include <stddef.h>
#include "GpuDevice/GpuDevice.h"

class GpuDrawItemWriterDesc {
public:
    GpuDrawItemWriterDesc();

    void SetNumCBuffers(int n);
    void SetNumVertexBuffers(int n);

private:
    friend class GpuDrawItemWriter;
    int m_nCBuffers;
    int m_nVertexBuffers;
};

class GpuDrawItemWriter {
public:
    typedef void* (*PFnAlloc)(size_t size, void* userdata);

    // Returns the number of bytes of memory needed for a draw call
    // created from the specified GpuDrawItemWriterDesc.
    static size_t SizeInBytes(const GpuDrawItemWriterDesc& desc);

    GpuDrawItemWriter();

    void Begin(const GpuDrawItemWriterDesc& desc,
               PFnAlloc alloc,
               void* userdata);
    GpuDrawItem* End();

    void SetPipelineState(GpuPipelineStateID state);
    void SetIndexBuffer(GpuBufferID buffer);
    void SetVertexBuffer(int index, GpuBufferID buffer, unsigned offset);
    void SetCBuffer(int index, GpuBufferID buffer);
    void SetDrawCall(GpuPrimitiveType primType, int first, int count);
    void SetDrawCallIndexed(GpuPrimitiveType primType,
                            int first,
                            int count,
                            unsigned indexBufOffset,
                            GpuIndexType indexType);

private:
    GpuDrawItemWriter(const GpuDrawItemWriter&);
    GpuDrawItemWriter& operator=(const GpuDrawItemWriter&);

    void ValidateVertexBuffers();
    void ValidateCBuffers();

    u8* m_drawItem;
    GpuDrawItemWriterDesc m_desc;
    u32 m_flags;
};

#endif // GPUDEVICE_GPUDRAWITEMWRITER_H
