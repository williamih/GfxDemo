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

struct GpuRegion {
    int x;
    int y;
    int width;
    int height;
};

struct GpuColor {
    float r;
    float g;
    float b;
    float a;
};

enum GpuPrimitiveType {
    GPU_PRIMITIVE_TRIANGLES,
    GPU_PRIMITIVE_TRIANGLE_STRIP,
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

enum GpuFillMode {
    GPU_FILL_MODE_WIREFRAME,
    GPU_FILL_MODE_SOLID,
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
DECLARE_PRIMITIVE_WRAPPER(u32, GpuTextureID);
DECLARE_PRIMITIVE_WRAPPER(u32, GpuSamplerID);

enum GpuBufferType {
    GPU_BUFFER_TYPE_VERTEX,
    GPU_BUFFER_TYPE_INDEX,
    GPU_BUFFER_TYPE_CONSTANT,
};

enum GpuBufferAccessMode {
    GPU_BUFFER_ACCESS_STATIC,
    GPU_BUFFER_ACCESS_DYNAMIC,
};

enum GpuPixelFormat {
    GPU_PIXEL_FORMAT_BGRA8888,
    GPU_PIXEL_FORMAT_DXT1,
    GPU_PIXEL_FORMAT_DXT3,
    GPU_PIXEL_FORMAT_DXT5,
    GPU_PIXEL_FORMAT_DEPTH_32,
    GPU_PIXEL_FORMAT_DEPTH_24_STENCIL_8,
};

enum GpuTextureType {
    GPU_TEXTURE_1D,
    GPU_TEXTURE_1D_ARRAY,
    GPU_TEXTURE_2D,
    GPU_TEXTURE_2D_ARRAY,
    GPU_TEXTURE_CUBE,
    GPU_TEXTURE_3D,
};

enum GpuTextureFlags {
    GPU_TEXTURE_FLAG_RENDER_TARGET = 1,
};

enum GpuSamplerAddressMode {
    GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE,
    GPU_SAMPLER_ADDRESS_MIRROR_CLAMP_TO_EDGE,
    GPU_SAMPLER_ADDRESS_REPEAT,
    GPU_SAMPLER_ADDRESS_MIRROR_REPEAT,
    GPU_SAMPLER_ADDRESS_CLAMP_TO_ZERO,
};

enum GpuSamplerFilterMode {
    GPU_SAMPLER_FILTER_NEAREST,
    GPU_SAMPLER_FILTER_LINEAR,
};

enum GpuSamplerMipFilter {
    GPU_SAMPLER_MIPFILTER_NOT_MIPMAPPED,
    GPU_SAMPLER_MIPFILTER_NEAREST,
    GPU_SAMPLER_MIPFILTER_LINEAR,
};

enum GpuRenderLoadAction {
    GPU_RENDER_LOAD_ACTION_LOAD,
    GPU_RENDER_LOAD_ACTION_CLEAR,
    GPU_RENDER_LOAD_ACTION_DISCARD,
};

enum GpuRenderStoreAction {
    GPU_RENDER_STORE_ACTION_STORE,
    GPU_RENDER_STORE_ACTION_DISCARD,
};

struct GpuSamplerDesc {
    GpuSamplerDesc();

    GpuSamplerAddressMode uAddressMode;
    GpuSamplerAddressMode vAddressMode;
    GpuSamplerAddressMode wAddressMode;
    GpuSamplerFilterMode minFilter;
    GpuSamplerFilterMode magFilter;
    GpuSamplerMipFilter mipFilter;
    int maxAnisotropy;
};

struct GpuPipelineStateDesc {
    GpuPipelineStateDesc();

    GpuShaderProgramID shaderProgram;
    u32 shaderStateBitfield;
    GpuInputLayoutID inputLayout;
    GpuCompareFunction depthCompare;
    bool depthWritesEnabled;
    GpuFillMode fillMode;
    GpuCullMode cullMode;
    GpuWindingOrder frontFaceWinding;
};

struct GpuRenderPassDesc {
    GpuRenderPassDesc();

    static const GpuRenderLoadAction DEFAULT_LOAD_ACTION = GPU_RENDER_LOAD_ACTION_DISCARD;
    static const GpuRenderStoreAction DEFAULT_STORE_ACTION = GPU_RENDER_STORE_ACTION_STORE;

    int numRenderTargets;

    // Must point to array of size: numRenderTargets.
    const GpuTextureID* renderTargets;

    // If any of the three pointers below is NULL, the relevant default values
    // are used.
    // Otherwise, each of these should point to an array of size:
    // max(1, numRenderTargets).
    // That is, if numRenderTargets == 0, then each of these three pointers
    // must point to an array of size 1, specifying the value to use for the
    // backbuffer.
    const GpuColor* clearColors;
    const GpuRenderLoadAction* colorLoadActions;
    const GpuRenderStoreAction* colorStoreActions;

    GpuTextureID depthStencilTarget;
    float clearDepth;
    GpuRenderLoadAction depthStencilLoadAction;
    GpuRenderStoreAction depthStencilStoreAction;
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

    // Textures
    bool TextureExists(GpuTextureID textureID) const;
    GpuTextureID TextureCreate(GpuTextureType type,
                               GpuPixelFormat pixelFormat,
                               u32 flags,
                               int width,
                               int height,
                               int depthOrArrayLength,
                               int nMipmapLevels);
    void TextureDestroy(GpuTextureID textureID);
    void TextureUpload(GpuTextureID textureID,
                       const GpuRegion& region,
                       int mipmapLevel,
                       int stride,
                       const void* bytes);

    // Samplers
    bool SamplerExists(GpuSamplerID samplerID) const;
    GpuSamplerID SamplerCreate(const GpuSamplerDesc& desc);
    void SamplerDestroy(GpuSamplerID samplerID);

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
#  define GPUDEVICE_UNREGISTER_DRAWITEM(dev, item) (dev).UnregisterDrawItem(item)
#else
#  define GPUDEVICE_UNREGISTER_DRAWITEM(dev, item) ((void)0)
#endif

#endif // GPUDEVICE_GPUDEVICE_H
