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
#include "Types.h"

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
    GPUINDEXTYPE_NONE,
    GPUINDEXTYPE_U16,
    GPUINDEXTYPE_U32,
};

struct GpuViewport {
    float x;
    float y;
    float width;
    float height;
};

// -----------------------------------------------------------------------------
// Draw items
// -----------------------------------------------------------------------------

struct GpuPipelineState {
    GpuShaderID vertexShader;
    GpuShaderID pixelShader;
    GpuInputLayoutID inputLayout;
};

struct GpuInputAssemblerState {
    struct VertexBufEntry {
        GpuBufferID bufferID;
        unsigned offset;
    };

    static const unsigned MAX_VERTEX_BUFFERS = 16;

    GpuPrimitiveType primType;
    GpuBufferID indexBufferID;
    VertexBufEntry vertexBuffers[MAX_VERTEX_BUFFERS];
    int nVertexBuffers;
};

struct GpuResourceList {
    GpuBufferID cbuffers[GPU_MAX_CBUFFERS];
    int nCBuffers;
};

struct GpuRenderPass {
    enum {
        FLAG_PERFORM_CLEAR = 1,
    };
    u32 flags;
    float clearR, clearG, clearB, clearA;
    float clearDepth;
};

struct GpuDrawItem {
    GpuPipelineStateID pipelineStateID;
    GpuInputAssemblerState inputAssembler;
    GpuResourceList resources;
    GpuViewport viewport;
    int first;
    int count;
    unsigned indexBufferOffset;
    GpuIndexType indexType;
};

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

    GpuShaderID CreateShader(GpuShaderType type, const char* code, size_t length);
    void DestroyShader(GpuShaderID shaderID);

    GpuBufferID CreateBuffer(GpuBufferType type,
                             GpuBufferAccessMode accessMode,
                             const void* data,
                             unsigned size);
    void DestroyBuffer(GpuBufferID bufferID);
    void* GetBufferContents(GpuBufferID bufferID);
    void FlushBufferRange(GpuBufferID bufferID, int start, int length);

    GpuPipelineStateID CreatePipelineStateObject(const GpuPipelineState& state);
    GpuRenderPassID CreateRenderPassObject(const GpuRenderPass& pass);
    GpuInputLayoutID CreateInputLayout(int nVertexAttribs,
                                       const GpuVertexAttribute* attribs,
                                       int nVertexBuffers,
                                       const unsigned* strides);

    void BeginRenderPass(GpuRenderPassID passID);
    void EndRenderPass();

    void Draw(const GpuDrawItem& item);

    void SceneBegin();
    void ScenePresent();

private:
    GpuDevice();
    ~GpuDevice();
    GpuDevice(const GpuDevice&);
    GpuDevice& operator=(const GpuDevice&);
};

#endif // GPUDEVICE_GPUDEVICE_H
