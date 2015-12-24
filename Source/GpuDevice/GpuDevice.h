/******************************************************************************
 *
 *   GpuDevice.h
 *
 ***/

/******************************************************************************
 *
 *   This file provides a low-level wrapper around the graphics API.
 *
 *   N.B. in this file, the order in which values are listed in each enum
 *   is important, as the enum values are used as indices for lookup tables.
 *   Do not change the order of the enum values!
 *
 ***/

#ifndef GPUDEVICE_GPUDEVICE_H
#define GPUDEVICE_GPUDEVICE_H

#include <stddef.h>
#include "Core/Types.h"

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

const unsigned GPU_MAX_CBUFFERS = 14;

// -----------------------------------------------------------------------------
// Graphics resources
// -----------------------------------------------------------------------------

DECLARE_PRIMITIVE_WRAPPER(u32, GpuShaderID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuBufferID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuTextureID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuPipelineStateID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuRenderPassID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuDescriptorSetID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuInputLayoutID);

enum GpuShaderType {
    GPUSHADERTYPE_VERTEX,
    GPUSHADERTYPE_PIXEL,
};

enum GpuBufferType {
    GPUBUFFERTYPE_VERTEX,
    GPUBUFFERTYPE_INDEX,
    GPUBUFFERTYPE_CONSTANT,
};

enum GpuBufferAccessMode {
    GPUBUFFER_ACCESS_STATIC,
    GPUBUFFER_ACCESS_DYNAMIC,
};

// -----------------------------------------------------------------------------
// Vertex formats
// -----------------------------------------------------------------------------

enum GpuVertexAttribFormat {
    GPUVERTEXATTRIB_HALF2,
    GPUVERTEXATTRIB_HALF3,
    GPUVERTEXATTRIB_HALF4,
    GPUVERTEXATTRIB_FLOAT,
    GPUVERTEXATTRIB_FLOAT2,
    GPUVERTEXATTRIB_FLOAT3,
    GPUVERTEXATTRIB_FLOAT4,
    GPUVERTEXATTRIB_UBYTE4_NORMALIZED,
};

struct GpuVertexAttribute {
    GpuVertexAttribFormat format;
    unsigned offset;
    int bufferSlot;
};

// -----------------------------------------------------------------------------
// Miscellaneous types
// -----------------------------------------------------------------------------

enum GpuPrimitiveType {
    GPUPRIMITIVE_TRIANGLES,
};

enum GpuIndexType {
    GPUINDEXTYPE_U16,
    GPUINDEXTYPE_U32,
};

struct GpuViewport {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    float zNear;
    float zFar;
};

enum GpuCompareFunction {
    GPU_COMPARE_NEVER,
    GPU_COMPARE_LESS,
    GPU_COMPARE_EQUAL,
    GPU_COMPARE_LESS_EQUAL,
    GPU_COMPARE_GREATER,
    GPU_COMPARE_NOT_EQUAL,
    GPU_COMPARE_GREATER_EQUAL,
    GPU_COMPARE_ALWAYS,
};

enum GpuCullMode {
    GPU_CULL_NONE,
    GPU_CULL_BACK,
    GPU_CULL_FRONT,
};

// -----------------------------------------------------------------------------
// Draw items
// -----------------------------------------------------------------------------

struct GpuPipelineStateDesc {
    GpuPipelineStateDesc();

    GpuShaderID vertexShader;
    GpuShaderID pixelShader;
    GpuInputLayoutID inputLayout;
    GpuCompareFunction depthCompare;
    bool depthWritesEnabled;
    GpuCullMode cullMode;
};

struct GpuRenderPassDesc {
    GpuRenderPassDesc();

    enum {
        FLAG_PERFORM_CLEAR = 1,
    };
    u32 flags;
    float clearR, clearG, clearB, clearA;
    float clearDepth;
};

class GpuDrawItem;

// -----------------------------------------------------------------------------
// GpuDeviceFormat
// -----------------------------------------------------------------------------

enum GpuPixelColorFormat {
    GPUPIXELCOLORFORMAT_RGBA8888,
};

enum GpuPixelDepthFormat {
    GPUPIXELDEPTHFORMAT_NONE,
    GPUPIXELDEPTHFORMAT_FLOAT24,
    GPUPIXELDEPTHFORMAT_FLOAT32,
};

struct GpuDeviceFormat {
    enum {
        FLAG_SCALE_RES_WITH_WINDOW_SIZE = 1,
    };

    GpuPixelColorFormat pixelColorFormat;
    GpuPixelDepthFormat pixelDepthFormat;
    int resolutionX;
    int resolutionY;
    u32 flags;
};

// -----------------------------------------------------------------------------
// The GpuDevice cross-platform API
// -----------------------------------------------------------------------------

class GpuDevice {
public:
    static GpuDevice* Create(const GpuDeviceFormat& format, void* osViewHandle);
    static void Destroy(GpuDevice* dev);

    void SetFormat(const GpuDeviceFormat& format);
    const GpuDeviceFormat& GetFormat() const;
    void OnWindowResized();

    bool ShaderIDExists(GpuShaderID shaderID) const;
    GpuShaderID CreateShader(GpuShaderType type, const char* code, size_t length);
    void DestroyShader(GpuShaderID shaderID);

    bool BufferIDExists(GpuBufferID bufferID) const;
    GpuBufferID CreateBuffer(GpuBufferType type,
                             GpuBufferAccessMode accessMode,
                             const void* data,
                             unsigned size);
    void DestroyBuffer(GpuBufferID bufferID);
    void* GetBufferContents(GpuBufferID bufferID);
    void FlushBufferRange(GpuBufferID bufferID, int start, int length);

    bool PipelineStateObjectIDExists(GpuPipelineStateID pipelineStateID) const;
    GpuPipelineStateID CreatePipelineStateObject(const GpuPipelineStateDesc& state);
    void DestroyPipelineStateObject(GpuPipelineStateID pipelineStateID);

    bool RenderPassObjectIDExists(GpuRenderPassID renderPassID) const;
    GpuRenderPassID CreateRenderPassObject(const GpuRenderPassDesc& pass);
    void DestroyRenderPassObject(GpuRenderPassID renderPassID);

    bool InputLayoutIDExists(GpuInputLayoutID inputLayoutID) const;
    GpuInputLayoutID CreateInputLayout(int nVertexAttribs,
                                       const GpuVertexAttribute* attribs,
                                       int nVertexBuffers,
                                       const unsigned* strides);
    void DestroyInputLayout(GpuInputLayoutID inputLayoutID);

    void Draw(const GpuDrawItem* const* items,
              int nItems,
              GpuRenderPassID renderPass,
              const GpuViewport& viewport);

    void SceneBegin();
    void ScenePresent();

#ifdef GPUDEVICE_DEBUG_MODE
    // Notifies the GpuDevice of the creation of a new GpuDrawItem.
    // This is called automatically by GpuDrawItemWriter, so typically clients
    // won't need to call this method.
    void RegisterDrawItem(const GpuDrawItem* item);

    // Notifies the GpuDevice that a GpuDrawItem is about to be destroyed.
    // This should be called by client code immediately before deleting a
    // GpuDrawItem or reusing its memory.
    void UnregisterDrawItem(const GpuDrawItem* item);
#endif // GPUDEVICE_DEBUG_MODE

private:
    GpuDevice();
    ~GpuDevice();
    GpuDevice(const GpuDevice&);
    GpuDevice& operator=(const GpuDevice&);
};

#ifdef GPUDEVICE_DEBUG_MODE
#  define GPUDEVICE_UNREGISTER_DRAWITEM(dev, item) dev->UnregisterDrawItem(item)
#else
#  define GPUDEVICE_UNREGISTER_DRAWITEM(dev, item) ((void)0)
#endif

#endif // GPUDEVICE_GPUDEVICE_H
