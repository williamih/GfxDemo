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
#include "Math/Matrix44.h"

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

const unsigned GPU_MAX_CBUFFERS = 14;

// -----------------------------------------------------------------------------
// Miscellaneous types
// -----------------------------------------------------------------------------

struct GpuViewport {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    float zNear;
    float zFar;
};

enum GpuPrimitiveType {
    GPU_PRIMITIVE_TRIANGLES,
};

enum GpuIndexType {
    GPU_INDEX_U16,
    GPU_INDEX_U32,
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

enum GpuWindingOrder {
    GPU_WINDING_CLOCKWISE,
    GPU_WINDING_COUNTER_CLOCKWISE,
};

// -----------------------------------------------------------------------------
// Graphics resources
// -----------------------------------------------------------------------------

DECLARE_PRIMITIVE_WRAPPER(u32, GpuShaderProgramID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuBufferID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuTextureID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuInputLayoutID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuPipelineStateID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuRenderPassID);

enum GpuBufferType {
    GPU_BUFFER_TYPE_VERTEX,
    GPU_BUFFER_TYPE_INDEX,
    GPU_BUFFER_TYPE_CONSTANT,
};

enum GpuBufferAccessMode {
    GPU_BUFFER_ACCESS_STATIC,
    GPU_BUFFER_ACCESS_DYNAMIC,
};

struct GpuPipelineStateDesc {
    GpuPipelineStateDesc();

    GpuShaderProgramID shaderProgram;
    u32 shaderStateBitfield;
    GpuInputLayoutID inputLayout;
    GpuCompareFunction depthCompare;
    bool depthWritesEnabled;
    GpuCullMode cullMode;
    GpuWindingOrder frontFaceWinding;
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

// -----------------------------------------------------------------------------
// Vertex formats
// -----------------------------------------------------------------------------

enum GpuVertexAttribFormat {
    GPU_VERTEX_ATTRIB_HALF2,
    GPU_VERTEX_ATTRIB_HALF3,
    GPU_VERTEX_ATTRIB_HALF4,
    GPU_VERTEX_ATTRIB_FLOAT,
    GPU_VERTEX_ATTRIB_FLOAT2,
    GPU_VERTEX_ATTRIB_FLOAT3,
    GPU_VERTEX_ATTRIB_FLOAT4,
    GPU_VERTEX_ATTRIB_UBYTE4_NORMALIZED,
};

struct GpuVertexAttribute {
    GpuVertexAttribFormat format;
    unsigned offset;
    int bufferSlot;
};

// -----------------------------------------------------------------------------
// GpuDrawItem -- client code only ever sees this forward declaration
// -----------------------------------------------------------------------------

class GpuDrawItem;

// -----------------------------------------------------------------------------
// GpuDeviceFormat
// -----------------------------------------------------------------------------

enum GpuPixelColorFormat {
    GPU_PIXEL_COLOR_FORMAT_RGBA8888,
};

enum GpuPixelDepthFormat {
    GPU_PIXEL_DEPTH_FORMAT_NONE,
    GPU_PIXEL_DEPTH_FORMAT_FLOAT24,
    GPU_PIXEL_DEPTH_FORMAT_FLOAT32,
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

    // Device format
    void SetFormat(const GpuDeviceFormat& format);
    const GpuDeviceFormat& GetFormat() const;
    void OnWindowResized();

    // Shaders
    bool ShaderProgramExists(GpuShaderProgramID shaderProgramID) const;
    GpuShaderProgramID ShaderProgramCreate(const char* data, size_t length);
    void ShaderProgramDestroy(GpuShaderProgramID shaderProgramID);

    // Buffers
    bool BufferExists(GpuBufferID bufferID) const;
    GpuBufferID BufferCreate(GpuBufferType type,
                             GpuBufferAccessMode accessMode,
                             const void* data,
                             unsigned size);
    void BufferDestroy(GpuBufferID bufferID);
    void* BufferGetContents(GpuBufferID bufferID);
    void BufferFlushRange(GpuBufferID bufferID, int start, int length);

    // Input layouts
    bool InputLayoutExists(GpuInputLayoutID inputLayoutID) const;
    GpuInputLayoutID InputLayoutCreate(int nVertexAttribs,
                                       const GpuVertexAttribute* attribs,
                                       int nVertexBuffers,
                                       const unsigned* strides);
    void InputLayoutDestroy(GpuInputLayoutID inputLayoutID);

    // Pipeline state objects
    bool PipelineStateExists(GpuPipelineStateID pipelineStateID) const;
    GpuPipelineStateID PipelineStateCreate(const GpuPipelineStateDesc& state);
    void PipelineStateDestroy(GpuPipelineStateID pipelineStateID);

    // Render passes
    bool RenderPassExists(GpuRenderPassID renderPassID) const;
    GpuRenderPassID RenderPassCreate(const GpuRenderPassDesc& pass);
    void RenderPassDestroy(GpuRenderPassID renderPassID);

    // Transforms
    static Matrix44 TransformCreateOrtho(float left, float right,
                                         float bot, float top,
                                         float near, float far);
    static Matrix44 TransformCreatePerspective(float left, float right,
                                               float bot, float top,
                                               float near, float far);

    // Submit an array of draw items
    void Draw(const GpuDrawItem* const* items,
              int nItems,
              GpuRenderPassID renderPass,
              const GpuViewport& viewport);

    // Scene begin/end functions
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
