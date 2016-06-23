#ifndef GPUDEVICE_GPUDRAWITEMWRITER_H
#define GPUDEVICE_GPUDRAWITEMWRITER_H

#include <stddef.h>
#include "GpuDevice/GpuDevice.h"

class GpuDrawItemWriterDesc {
public:
    GpuDrawItemWriterDesc();

    void SetNumCBuffers(int n);
    void SetNumVertexBuffers(int n);
    void SetNumTextures(int n);
    void SetNumSamplers(int n);

    int NumCBuffers() const { return m_nCBuffers; }
    int NumVertexBuffers() const { return m_nVertexBuffers; }
    int NumTextures() const { return m_nTextures; }
    int NumSamplers() const { return m_nSamplers; }

private:
    int m_nCBuffers;
    int m_nVertexBuffers;
    int m_nTextures;
    int m_nSamplers;
};

class GpuDrawItemWriter {
public:

    // Returns the number of bytes of memory needed for a draw call
    // created from the specified GpuDrawItemWriterDesc.
    static size_t SizeInBytes(const GpuDrawItemWriterDesc& desc);

    GpuDrawItemWriter();

    void Begin(GpuDevice* device, const GpuDrawItemWriterDesc& desc, void* memory);
    GpuDrawItem* End();

    void SetPipelineState(GpuPipelineStateID state);
    void SetIndexBuffer(GpuBufferID buffer);

    void SetVertexBuffer(int index, GpuBufferID buffer, unsigned offset);
    void SetCBuffer(int index, GpuBufferID buffer);
    void SetTexture(int index, GpuTextureID texture);
    void SetSampler(int index, GpuSamplerID sampler);

    void SetDrawCall(GpuPrimitiveType primType, int first, int count);
    void SetDrawCallIndexed(GpuPrimitiveType primType,
                            int first,
                            int count,
                            unsigned indexBufOffset,
                            GpuIndexType indexType);

private:
    GpuDrawItemWriter(const GpuDrawItemWriter&);
    GpuDrawItemWriter& operator=(const GpuDrawItemWriter&);

    GpuDevice* m_device;
    GpuDrawItem* m_drawItem;
    GpuDrawItemWriterDesc m_desc;
    u32 m_flags;
};

#endif // GPUDEVICE_GPUDRAWITEMWRITER_H
