/******************************************************************************
 *
 *   GpuDrawItem.h
 *
 ***/

/******************************************************************************
 *
 *   This file is private to the GpuDevice module.
 *   Do NOT use this file in client code.
 *
 ***/

#ifndef GPUDEVICE_GPUDRAWITEM_H
#define GPUDEVICE_GPUDRAWITEM_H

#include "Core/Types.h"
#include "GpuDevice/GpuDevice.h"

class GpuDrawItem {
public:
    static const u32 NUM_PRIMITIVE_TYPE_BITS = 3;

    GpuPrimitiveType GetPrimitiveType() const
    {
        return (GpuPrimitiveType)(flags & 0x7);
    }

    GpuIndexType GetIndexType() const
    {
        if (flags & 0x8)
            return GPU_INDEX_U32;
        return GPU_INDEX_U16;
    }

    bool IsIndexed() const
    {
        return (flags & 0x10) != 0;
    }

    void SetPrimitiveType(GpuPrimitiveType type)
    {
        ASSERT((u32)type < (1 << NUM_PRIMITIVE_TYPE_BITS));
        flags &= ~(0x7);
        flags |= (u16)type;
    }

    void SetIndexType(GpuIndexType type)
    {
        switch (type) {
            case GPU_INDEX_U16:
                flags &= ~(0x8);
                break;
            case GPU_INDEX_U32:
                flags |= 0x8;
                break;
            default:
                break;
        }
    }

    void EnableIndexed()
    {
        flags |= 0x10;
    }

    u32* VertexBufferOffsets() const
    { return (u32*)(this + 1); }

    u16* VertexBuffers() const
    { return (u16*)(VertexBufferOffsets() + nVertexBuffers); }

    u16* CBuffers() const
    { return (u16*)(VertexBuffers() + nVertexBuffers); }

    u16* Textures() const
    { return (u16*)(CBuffers() + nCBuffers); }

    u16* Samplers() const
    { return (u16*)(Textures() + nTextures); }

    // Description of flags (16-bit):
    //   Bits 0-2 contain the primitive type.
    //   Bit 3 contains the index type (16 vs 32 bit)
    //   Bit 4 is 1 if indexed drawing is used, otherwise 0 for non-indexed.

    u16 pipelineStateIdx;
    u16 flags;
    u16 indexBufferIdx;
    u8 nVertexBuffers;
    u8 nCBuffers;
    u8 nTextures;
    u8 nSamplers;
    u16 _pad;
    u32 first;
    u32 count;
    u32 indexBufferOffset;
    // Following this are:
    //   (1) vertex buffers offsets: array of size nVertexBuffers of u32
    //   (2) vertex buffer indices: array of size nVertexBuffers of u16
    //   (3) cbuffers indices: array of size nCBuffers of u16
    //   (4) texture indices: array of size nTextures of u16
    //   (5) sampler indices: array of size nSamplers of u16
};

#endif // GPUDEVICE_GPUDRAWITEM_H
