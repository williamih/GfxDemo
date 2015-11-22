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

struct GpuDrawItemHeader {
    struct VertexBufEntry {
        u32 bufferID;
        u32 offset;
    };

    inline GpuPrimitiveType GetPrimitiveType() const
    {
        switch (flags & 0x7) {
            case 0: return GPUPRIMITIVE_TRIANGLES;
            default: return GPUPRIMITIVE_TRIANGLES;
        }
    }

    inline GpuIndexType GetIndexType() const
    {
        if (flags & 0x8)
            return GPUINDEXTYPE_U32;
        return GPUINDEXTYPE_U16;
    }

    inline bool IsIndexed() const
    {
        return (flags & 0x10) != 0;
    }

    inline void SetPrimitiveType(GpuPrimitiveType type)
    {
        switch (type) {
            case GPUPRIMITIVE_TRIANGLES:
                flags &= ~(0x7);
                break;
            default: break;
        }
    }

    inline void SetIndexType(GpuIndexType type)
    {
        switch (type) {
            case GPUINDEXTYPE_U16:
                flags &= ~(0x8);
                break;
            case GPUINDEXTYPE_U32:
                flags |= 0x8;
                break;
            default:
                break;
        }
    }

    inline void EnableIndexed()
    {
        flags |= 0x10;
    }

    inline VertexBufEntry* GetVertexBufferArray() const
    {
        u8* ptr = (u8*)this + sizeof(GpuDrawItemHeader);
        return (VertexBufEntry*)ptr;
    }

    inline u32* GetCBufferArray() const
    {
        u8* ptr = (u8*)this + sizeof(GpuDrawItemHeader);
        ptr += nVertexBuffers * sizeof(VertexBufEntry);
        return (u32*)ptr;
    }

    u32 pipelineStateID;
    u32 flags; // contains the primitive type and the index type
    u32 indexBufferID;
    u16 nVertexBuffers;
    u16 nCBuffers;
    f32 zNear;
    f32 zFar;
    u32 first;
    u32 count;
    u32 indexBufferOffset;
    // Following this are:
    //   (1) vertex buffers: array of size nVertexBuffers of VertexBufEntry
    //   (2) cbuffers: array of size nCBuffers of u32
};

#endif // GPUDEVICE_GPUDRAWITEM_H
