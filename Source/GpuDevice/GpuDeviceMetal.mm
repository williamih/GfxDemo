#ifdef GPUDEVICE_API_METAL

#include "GpuDevice/GpuDevice.h"

#include <stdio.h>

#import <Cocoa/Cocoa.h> // for NSView
#import <QuartzCore/QuartzCore.h> // for CAMetalLayer
#import <Metal/Metal.h>

#include "Core/IDLookupTable.h"
#include "Core/Macros.h"
#include "GpuDevice/GpuDrawItem.h"
#include "GpuDevice/GpuShaderLoad.h"
#include "GpuDevice/GpuShaderPermutations.h"

#define FOURCC(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

const u32 STREAM_RING_BUF_SIZE = 2 * 1024 * 1024; // 2 MB

// All streaming-mode and dynamic-mode buffers have their size rounded up to a
// multiple of this number of bytes.
// Note: As per the MTLRenderCommandEncoder documentation, only a 16 byte
// alignment is required. However, we use a 64 byte alignment as this is
// believed to be more optimal.
const u32 BUF_ALIGNMENT = 64;

// Alignment for constant buffers. The value of 256 bytes is required, as
// specified by the MTLRenderCommandEncoder documentation.
const u32 CONSTANT_BUF_ALIGNMENT = 256;

// We use double-buffering, so there are always two frames in-flight.
const int FRAMES_IN_FLIGHT = 2;

// -----------------------------------------------------------------------------
// Lookup tables for miscellaneous types
// -----------------------------------------------------------------------------

static const MTLPrimitiveType s_metalPrimitiveTypes[] = {
    MTLPrimitiveTypeTriangle, // GPU_PRIMITIVE_TRIANGLES
    MTLPrimitiveTypeTriangleStrip, // GPU_PRIMITIVE_TRIANGLE_STRIP
};

static const MTLIndexType s_metalIndexTypes[] = {
    MTLIndexTypeUInt16, // GPU_INDEX_U16
    MTLIndexTypeUInt32, // GPU_INDEX_U32
};

static const NSUInteger s_indexTypeByteSizes[] = {
    2, // GPU_INDEX_U16
    4, // GPU_INDEX_U32
};

static const MTLCompareFunction s_metalCompareFunctions[] = {
    MTLCompareFunctionNever, // GPU_COMPARE_NEVER
    MTLCompareFunctionLess, // GPU_COMPARE_LESS
    MTLCompareFunctionEqual, // GPU_COMPARE_EQUAL
    MTLCompareFunctionLessEqual, // GPU_COMPARE_LESS_EQUAL
    MTLCompareFunctionGreater, // GPU_COMPARE_GREATER
    MTLCompareFunctionNotEqual, // GPU_COMPARE_NOT_EQUAL
    MTLCompareFunctionGreaterEqual, // GPU_COMPARE_GREATER_EQUAL
    MTLCompareFunctionAlways, // GPU_COMPARE_ALWAYS
};

static const MTLTriangleFillMode s_metalFillModes[] = {
    MTLTriangleFillModeLines, // GPU_FILL_MODE_WIREFRAME
    MTLTriangleFillModeFill, // GPU_FILL_MODE_SOLID
};

static const MTLCullMode s_metalCullModes[] = {
    MTLCullModeNone, // GPU_CULL_NONE
    MTLCullModeBack, // GPU_CULL_BACK
    MTLCullModeFront, // GPU_CULL_FRONT
};

static const MTLWinding s_metalWindingOrders[] = {
    MTLWindingClockwise, // GPU_WINDING_CLOCKWISE
    MTLWindingCounterClockwise, // GPU_WINDING_COUNTER_CLOCKWISE
};

static const MTLLoadAction s_metalLoadActions[] = {
    MTLLoadActionLoad, // GPU_RENDER_LOAD_ACTION_LOAD
    MTLLoadActionClear, // GPU_RENDER_LOAD_ACTION_CLEAR
    MTLLoadActionDontCare, // GPU_RENDER_LOAD_ACTION_DISCARD
};

static const MTLStoreAction s_metalStoreActions[] = {
    MTLStoreActionStore, // GPU_RENDER_STORE_ACTION_STORE
    MTLStoreActionDontCare, // GPU_RENDER_STORE_ACTION_DISCARD
};

static const MTLBlendFactor s_metalBlendFactors[] = {
    MTLBlendFactorZero, // GPU_BLEND_ZERO
    MTLBlendFactorOne, // GPU_BLEND_ONE
    MTLBlendFactorSourceColor, // GPU_BLEND_SRC_COLOR
    MTLBlendFactorOneMinusSourceColor, // GPU_BLEND_ONE_MINUS_SRC_COLOR
    MTLBlendFactorDestinationColor, // GPU_BLEND_DST_COLOR
    MTLBlendFactorOneMinusDestinationColor, // GPU_BLEND_ONE_MINUS_DST_COLOR
    MTLBlendFactorSourceAlpha, // GPU_BLEND_SRC_ALPHA
    MTLBlendFactorOneMinusSourceAlpha, // GPU_BLEND_ONE_MINUS_SRC_ALPHA
    MTLBlendFactorDestinationAlpha, // GPU_BLEND_DST_ALPHA
    MTLBlendFactorOneMinusDestinationAlpha, // GPU_BLEND_ONE_MINUS_DST_ALPHA
};

// -----------------------------------------------------------------------------
// Lookup tables for buffers
// -----------------------------------------------------------------------------

static const u32 s_bufferAlignments[] = {
    BUF_ALIGNMENT, // GPU_BUFFER_TYPE_VERTEX
    BUF_ALIGNMENT, // GPU_BUFFER_TYPE_INDEX
    CONSTANT_BUF_ALIGNMENT, // GPU_BUFFER_TYPE_CONSTANT
};

// -----------------------------------------------------------------------------
// Lookup tables for textures
// -----------------------------------------------------------------------------

static const MTLPixelFormat s_metalPixelFormats[] = {
    MTLPixelFormatBGRA8Unorm, // GPU_PIXEL_FORMAT_BGRA8888
    MTLPixelFormatBC1_RGBA, // GPU_PIXEL_FORMAT_DXT1
    MTLPixelFormatBC2_RGBA, // GPU_PIXEL_FORMAT_DXT3
    MTLPixelFormatBC3_RGBA, // GPU_PIXEL_FORMAT_DXT5
    MTLPixelFormatDepth32Float, // GPU_PIXEL_FORMAT_DEPTH_32
    MTLPixelFormatDepth24Unorm_Stencil8, // GPU_PIXEL_FORMAT_DEPTH_24_STENCIL_8
};

static const MTLTextureType s_metalTextureTypes[] = {
    MTLTextureType1D, // GPU_TEXTURE_1D
    MTLTextureType1DArray, // GPU_TEXTURE_1D_ARRAY
    MTLTextureType2D, // GPU_TEXTURE_2D
    MTLTextureType2DArray, // GPU_TEXTURE_2D_ARRAY
    MTLTextureTypeCube, // GPU_TEXTURE_CUBE
    MTLTextureType3D, // GPU_TEXTURE_3D
};

static const bool s_textureTypeIsArray[] = {
    false, // GPU_TEXTURE_1D
    true, // GPU_TEXTURE_1D_ARRAY
    false, // GPU_TEXTURE_2D
    true, // GPU_TEXTURE_2D_ARRAY
    false, // GPU_TEXTURE_CUBE
    false, // GPU_TEXTURE_3D
};

// -----------------------------------------------------------------------------
// Lookup tables for samplers
// -----------------------------------------------------------------------------

static const MTLSamplerAddressMode s_metalSamplerAddressModes[] = {
    MTLSamplerAddressModeClampToEdge, // GPU_SAMPLER_ADDRESS_CLAMP_TO_EDGE
    MTLSamplerAddressModeMirrorClampToEdge, // GPU_SAMPLER_ADDRESS_MIRROR_CLAMP_TO_EDGE
    MTLSamplerAddressModeRepeat, // GPU_SAMPLER_ADDRESS_REPEAT
    MTLSamplerAddressModeMirrorRepeat, // GPU_SAMPLER_ADDRESS_MIRROR_REPEAT
    MTLSamplerAddressModeClampToZero, // GPU_SAMPLER_ADDRESS_CLAMP_TO_ZERO
};

static const MTLSamplerMinMagFilter s_metalSamplerMinMagFilters[] = {
    MTLSamplerMinMagFilterNearest, // GPU_SAMPLER_FILTER_NEAREST
    MTLSamplerMinMagFilterLinear, // GPU_SAMPLER_FILTER_LINEAR
};

static const MTLSamplerMipFilter s_metalSamplerMipFilters[] = {
    MTLSamplerMipFilterNotMipmapped, // GPU_SAMPLER_MIPFILTER_NOT_MIPMAPPED
    MTLSamplerMipFilterNearest, // GPU_SAMPLER_MIPFILTER_NEAREST
    MTLSamplerMipFilterLinear, // GPU_SAMPLER_MIPFILTER_LINEAR
};

// -----------------------------------------------------------------------------
// Lookup tables for vertex formats
// -----------------------------------------------------------------------------

static const MTLVertexFormat s_metalVertexAttribFormats[] = {
    MTLVertexFormatHalf2, // GPU_VERTEX_ATTRIB_HALF2
    MTLVertexFormatHalf3, // GPU_VERTEX_ATTRIB_HALF3
    MTLVertexFormatHalf4, // GPU_VERTEX_ATTRIB_HALF4
    MTLVertexFormatFloat, // GPU_VERTEX_ATTRIB_FLOAT
    MTLVertexFormatFloat2, // GPU_VERTEX_ATTRIB_FLOAT2
    MTLVertexFormatFloat3, // GPU_VERTEX_ATTRIB_FLOAT3
    MTLVertexFormatFloat4, // GPU_VERTEX_ATTRIB_FLOAT4
    MTLVertexFormatUChar4Normalized, // GPU_VERTEX_ATTRIB_UBYTE4_NORMALIZED
};

// -----------------------------------------------------------------------------
// Lookup tables for the device format
// -----------------------------------------------------------------------------

static const MTLPixelFormat s_metalColorPixelFormats[] = {
    MTLPixelFormatBGRA8Unorm, // GPU_PIXEL_COLOR_FORMAT_RGBA8888
};

// Metal doesn't support a 24-bit depth buffer. We use 32-bit no matter what
// the client requests.
static const MTLPixelFormat s_metalDepthPixelFormats[] = {
    MTLPixelFormatInvalid, // GPU_PIXEL_DEPTH_FORMAT_NONE
    MTLPixelFormatDepth32Float, // GPU_PIXEL_DEPTH_FORMAT_FLOAT24
    MTLPixelFormatDepth32Float, // GPU_PIXEL_DEPTH_FORMAT_FLOAT32
};

// -----------------------------------------------------------------------------
// GpuDeviceMetal class declaration
// -----------------------------------------------------------------------------

class GpuDeviceMetal {
public:
    GpuDeviceMetal(const GpuDeviceFormat& format, void* osViewHandle);
    ~GpuDeviceMetal();

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
                             unsigned size,
                             int maxUpdatesPerFrame);
    void BufferDestroy(GpuBufferID bufferID);
    void BufferStreamResize(GpuBufferID bufferID, unsigned newSize);

private:
    void* DynamicBufferMap(GpuBufferID bufferID);
    void DynamicBufferUnmap(GpuBufferID bufferID);
    void* StreamBufferMap(GpuBufferID bufferID);
    void StreamBufferUnmap(GpuBufferID bufferID);
public:
    void* BufferMap(GpuBufferID bufferID);
    void BufferUnmap(GpuBufferID bufferID);

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
private:
    id<MTLRenderPipelineState> CreateMTLRenderPipelineState(const GpuPipelineStateDesc& state);
    id<MTLDepthStencilState> CreateMTLDepthStencilState(const GpuPipelineStateDesc& state);
public:
    void PipelineStateDestroy(GpuPipelineStateID pipelineStateID);

    // Render passes
    bool RenderPassExists(GpuRenderPassID renderPassID) const;
    GpuRenderPassID RenderPassCreate(const GpuRenderPassDesc& pass);
    void RenderPassDestroy(GpuRenderPassID renderPassID);

    // Submit an array of draw items
private:
    id<MTLRenderCommandEncoder> PreDraw(GpuRenderPassID passID, const GpuViewport& viewport);
    void PostDraw(id<MTLRenderCommandEncoder> encoder);
public:
    void Draw(const GpuDrawItem* const* items,
              int nItems,
              GpuRenderPassID renderPass,
              const GpuViewport& viewport);

    // Scene begin/end functions
    void SceneBegin();
    void ScenePresent();

#ifdef GPUDEVICE_DEBUG_MODE
    void DrawItem_UpdateRefCounts(const GpuDrawItem* item, int increment);
    void RegisterDrawItem(const GpuDrawItem* item);
    void UnregisterDrawItem(const GpuDrawItem* item);
#endif

private:
    GpuDeviceMetal(const GpuDeviceMetal&);
    GpuDeviceMetal& operator=(const GpuDeviceMetal&);

    struct PermutationApiData {
        PermutationApiData()
            : library(NULL)
            , vertexFunction(NULL)
            , fragmentFunction(NULL)
        {}

        bool IsInUse() const
        {
            return library != NULL;
        }

        void LoadVertexShader(GpuDevice& device, const char* code, int length)
        {
            GpuDeviceMetal& devMTL = (GpuDeviceMetal&)device;

            void* codeCopy = malloc(length);
            memcpy(codeCopy, code, length);
            dispatch_data_t data = dispatch_data_create(codeCopy, length, NULL,
                                                        DISPATCH_DATA_DESTRUCTOR_FREE);

            NSError* error;
            library = [devMTL.m_device newLibraryWithData:data error:&error];
            if (error) {
                FATAL("GpuDeviceMetal: Failed to create shader: %s",
                      error.localizedDescription.UTF8String);
            }

            vertexFunction = [library newFunctionWithName:@"VertexMain"];
            fragmentFunction = [library newFunctionWithName:@"PixelMain"];
        }

        void LoadPixelShader(GpuDevice& device, const char* code, int length)
        {}

        void Release()
        {
            [library release];
            [vertexFunction release];
            [fragmentFunction release];
            library = NULL;
            vertexFunction = NULL;
            fragmentFunction = NULL;
        }

        id<MTLLibrary> library;
        id<MTLFunction> vertexFunction;
        id<MTLFunction> fragmentFunction;
    };

    struct ShaderProgram {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
#endif
        u32 idxFirstPermutation;
    };

    struct Buffer {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
#endif
        // TODO: Could be worth compressing the size of this struct a bit more
        id<MTLBuffer> buffer;
        GpuBufferType type;
        GpuBufferAccessMode accessMode;
        int maxUpdatesPerFrame;
        u32 size;
        u32 bufOffset;
    };

    struct PipelineStateObj {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
        u32 dbg_shaderProgram;
        u32 dbg_inputLayout;
#endif
        id<MTLRenderPipelineState> state;
        id<MTLDepthStencilState> depthStencilState;
        MTLTriangleFillMode triangleFillMode;
        MTLCullMode cullMode;
        MTLWinding frontFaceWinding;
    };

    struct RenderPassObj {
        MTLRenderPassDescriptor* descriptor;
        bool usesRenderTarget;
    };

    struct InputLayout {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
#endif
        MTLVertexDescriptor* descriptor;
    };

    struct Texture {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
#endif
        id<MTLTexture> texture;
    };

    struct Sampler {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
#endif
        id<MTLSamplerState> samplerState;
    };

    void CreateOrDestroyDepthBuffers();
    CAMetalLayer* GetCAMetalLayer() const;

    GpuDeviceFormat m_deviceFormat;
    NSView* m_view;
    CGSize m_viewSize;
    int m_frameNumber;
    int m_doubleBufferIndex;

    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;

    id<MTLTexture> m_depthBufs[2];
    id<MTLCommandBuffer> m_commandBuffer;
    id<MTLCommandBuffer> m_prevCommandBuffer;

    id<CAMetalDrawable> m_currentDrawable;

    // We always keep two copies of each buffer -- one for each frame.
    // Hence the size of this vector is always even.
    std::vector<id<MTLBuffer> > m_streamRingBufs;
    u32 m_streamBufCursor;
    int m_streamBufIndex;

    GpuShaderPermutations<PermutationApiData> m_permutations;
    IDLookupTable<ShaderProgram, GpuShaderProgramID::Type, 16, 16> m_shaderProgramTable;
    IDLookupTable<Buffer, GpuBufferID::Type, 16, 16> m_bufferTable;
    IDLookupTable<PipelineStateObj, GpuPipelineStateID::Type, 16, 16> m_pipelineStateTable;
    IDLookupTable<RenderPassObj, GpuRenderPassID::Type, 16, 16> m_renderPassTable;
    IDLookupTable<InputLayout, GpuInputLayoutID::Type, 16, 16> m_inputLayoutTable;
    IDLookupTable<Texture, GpuTextureID::Type, 16, 16> m_textureTable;
    IDLookupTable<Sampler, GpuSamplerID::Type, 16, 16> m_samplerTable;

    int m_dbg_shaderCount;
    int m_dbg_bufferCount;
    int m_dbg_textureCount;
    int m_dbg_samplerCount;
    int m_dbg_psoCount;
    int m_dbg_renderPassCount;
    int m_dbg_inputLayoutCount;
};

// -----------------------------------------------------------------------------
// Static functions used throughout the code
// -----------------------------------------------------------------------------

static id<MTLBuffer> CreateStreamRingBuffer(id<MTLDevice> device)
{
    MTLResourceOptions options = MTLResourceStorageModeShared
        | MTLResourceCPUCacheModeWriteCombined;
    return [device newBufferWithLength:STREAM_RING_BUF_SIZE options:options];
}

static u32 RoundUp(u32 numToRound, u32 multiple)
{
    ASSERT(multiple != 0);
    return ((numToRound + multiple - 1) / multiple) * multiple;
}

// -----------------------------------------------------------------------------
// GpuDeviceMetal implementation
// -----------------------------------------------------------------------------

GpuDeviceMetal::GpuDeviceMetal(const GpuDeviceFormat& format, void* osViewHandle)
    : m_deviceFormat(format)
    , m_view((NSView*)osViewHandle)
    , m_viewSize([m_view bounds].size)
    , m_frameNumber(0)
    , m_doubleBufferIndex(0)

    , m_device(nil)
    , m_commandQueue(nil)

    , m_depthBufs()
    , m_commandBuffer(nil)
    , m_prevCommandBuffer(nil)

    , m_currentDrawable(nil)

    , m_streamRingBufs()
    , m_streamBufCursor(0)
    , m_streamBufIndex()

    , m_permutations()
    , m_shaderProgramTable()
    , m_bufferTable()
    , m_pipelineStateTable()
    , m_renderPassTable()
    , m_inputLayoutTable()

    , m_dbg_shaderCount(0)
    , m_dbg_bufferCount(0)
    , m_dbg_textureCount(0)
    , m_dbg_samplerCount(0)
    , m_dbg_psoCount(0)
    , m_dbg_renderPassCount(0)
    , m_dbg_inputLayoutCount(0)
{
    m_depthBufs[0] = m_depthBufs[1] = nil;

    if (![[m_view layer] isKindOfClass:[CAMetalLayer class]])
        FATAL("GpuDeviceMetal: View needs to have a CAMetalLayer");

    m_device = MTLCreateSystemDefaultDevice();
    GetCAMetalLayer().device = m_device;
    GetCAMetalLayer().drawableSize = CGSizeMake(format.resolutionX, format.resolutionY);
    GetCAMetalLayer().pixelFormat = s_metalColorPixelFormats[format.pixelColorFormat];
    CreateOrDestroyDepthBuffers();

    m_commandQueue = [m_device newCommandQueue];

    m_commandBuffer = [[m_commandQueue commandBuffer] retain];

    // Initialize with two ring buffers for streaming,
    // i.e. one for each in-flight frame.
    m_streamRingBufs.push_back(CreateStreamRingBuffer(m_device));
    m_streamRingBufs.push_back(CreateStreamRingBuffer(m_device));
}

GpuDeviceMetal::~GpuDeviceMetal()
{
    for (size_t i = 0; i < m_streamRingBufs.size(); ++i) {
        [m_streamRingBufs[i] release];
    }

    [m_commandBuffer release];
    [m_prevCommandBuffer release];

    [m_depthBufs[0] release];
    [m_depthBufs[1] release];

    [m_commandQueue release];
    [m_device release];

    if (m_dbg_shaderCount != 0) {
        fprintf(stderr,
                "GpuDeviceMetal: warning - %d shader(s) not destroyed\n",
                m_dbg_shaderCount);
    }
    if (m_dbg_bufferCount != 0) {
        fprintf(stderr,
                "GpuDeviceMetal: warning - %d buffer(s) not destroyed\n",
                m_dbg_bufferCount);
    }
    if (m_dbg_textureCount != 0) {
        fprintf(stderr,
                "GpuDeviceMetal: warning - %d textures(s) not destroyed\n",
                m_dbg_textureCount);
    }
    if (m_dbg_samplerCount != 0) {
        fprintf(stderr,
                "GpuDeviceMetal: warning - %d sampler(s) not destroyed\n",
                m_dbg_samplerCount);
    }
    if (m_dbg_psoCount != 0) {
        fprintf(stderr,
                "GpuDeviceMetal: warning - %d pipeline state object(s) "
                "not destroyed\n", m_dbg_psoCount);
    }
    if (m_dbg_renderPassCount != 0) {
        fprintf(stderr,
                "GpuDeviceMetal: warning - %d render pass object(s) "
                "not destroyed\n", m_dbg_renderPassCount);
    }
    if (m_dbg_inputLayoutCount != 0) {
        fprintf(stderr,
                "GpuDeviceMetal: warning - %d input layout(s) "
                "not destroyed\n", m_dbg_inputLayoutCount);
    }
}

void GpuDeviceMetal::CreateOrDestroyDepthBuffers()
{
    // Release the current depth buffers.
    [m_depthBufs[0] release];
    [m_depthBufs[1] release];
    m_depthBufs[0] = m_depthBufs[1] = nil;

    // If the client didn't request a depth buffer, then we are done.
    if (m_deviceFormat.pixelDepthFormat == GPU_PIXEL_DEPTH_FORMAT_NONE)
        return;

    MTLPixelFormat format = s_metalDepthPixelFormats[m_deviceFormat.pixelDepthFormat];

    MTLTextureDescriptor* desc;
    desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
                                                              width:m_deviceFormat.resolutionX
                                                             height:m_deviceFormat.resolutionY
                                                          mipmapped:NO];
    desc.storageMode = MTLStorageModePrivate;
    desc.usage |= MTLTextureUsageRenderTarget;

    m_depthBufs[0] = [m_device newTextureWithDescriptor:desc];
    m_depthBufs[1] = [m_device newTextureWithDescriptor:desc];
}

CAMetalLayer* GpuDeviceMetal::GetCAMetalLayer() const
{
    return (CAMetalLayer*)[m_view layer];
}

void GpuDeviceMetal::SetFormat(const GpuDeviceFormat& format)
{
    m_deviceFormat = format;
    GetCAMetalLayer().drawableSize = CGSizeMake(format.resolutionX, format.resolutionY);
    GetCAMetalLayer().pixelFormat = s_metalColorPixelFormats[format.pixelColorFormat];
    CreateOrDestroyDepthBuffers();
}

const GpuDeviceFormat& GpuDeviceMetal::GetFormat() const
{
    return m_deviceFormat;
}

void GpuDeviceMetal::OnWindowResized()
{
    CGSize newSize = [m_view bounds].size;
    if (m_deviceFormat.flags & GpuDeviceFormat::FLAG_SCALE_RES_WITH_WINDOW_SIZE) {
        CGFloat widthRatio = newSize.width / m_viewSize.width;
        CGFloat heightRatio = newSize.height / m_viewSize.height;
        m_deviceFormat.resolutionX = (int)(m_deviceFormat.resolutionX * widthRatio);
        m_deviceFormat.resolutionY = (int)(m_deviceFormat.resolutionY * heightRatio);
        GetCAMetalLayer().drawableSize = CGSizeMake(m_deviceFormat.resolutionX,
                                                    m_deviceFormat.resolutionY);
    }
    m_viewSize = newSize;

    CreateOrDestroyDepthBuffers();
}

bool GpuDeviceMetal::ShaderProgramExists(GpuShaderProgramID shaderProgramID) const
{
    return m_shaderProgramTable.Has(shaderProgramID);
}

GpuShaderProgramID GpuDeviceMetal::ShaderProgramCreate(const char* data, size_t length)
{
    GpuShaderProgramID shaderProgramID(m_shaderProgramTable.Add());
    ShaderProgram& program = m_shaderProgramTable.Lookup(shaderProgramID);
#ifdef GPUDEVICE_DEBUG_MODE
    program.dbg_refCount = 0;
#endif

    program.idxFirstPermutation = GpuShaderLoad::LoadShader(
        data,
        (int)length,
        FOURCC('M', 'E', 'T', 'L'),
        *(GpuDevice*)this,
        m_permutations
    );

    ++m_dbg_shaderCount;

    return shaderProgramID;
}

void GpuDeviceMetal::ShaderProgramDestroy(GpuShaderProgramID shaderProgramID)
{
    ASSERT(ShaderProgramExists(shaderProgramID));
    ShaderProgram& program = m_shaderProgramTable.Lookup(shaderProgramID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (program.dbg_refCount != 0) {
        FATAL("Can't destroy shader program as it still has %d pipeline state "
              "object(s) referencing it", program.dbg_refCount);
    }
#endif

    m_permutations.ReleaseChain(program.idxFirstPermutation);

    m_shaderProgramTable.Remove(shaderProgramID);

    --m_dbg_shaderCount;
}

bool GpuDeviceMetal::BufferExists(GpuBufferID bufferID) const
{
    return m_bufferTable.Has(bufferID);
}

static id<MTLBuffer> CreateStaticBuffer(id<MTLDevice> device,
                                        id<MTLCommandBuffer> commandBuffer,
                                        const void* data, unsigned size)
{
    MTLResourceOptions options =
        MTLResourceStorageModePrivate | MTLResourceCPUCacheModeDefaultCache;

    if (data == NULL)
        return [device newBufferWithLength:size options:options];

    // Create a shared-mode buffer, fill it with data and then blit it into a
    // private-mode buffer.

    MTLResourceOptions sharedOptions =
        MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache;

    id<MTLBuffer> sharedBuffer = [device newBufferWithBytes:data
                                                     length:size
                                                    options:sharedOptions];

    id<MTLBuffer> result = [device newBufferWithLength:size options:options];

    id<MTLBlitCommandEncoder> encoder = [commandBuffer blitCommandEncoder];
    [encoder copyFromBuffer:sharedBuffer
               sourceOffset:0
                   toBuffer:result
          destinationOffset:0
                       size:size];
    [encoder endEncoding];

    [sharedBuffer release];

    return result;
}

// TODO: it could be worth using MTLResourceStorageModeManaged instead
// of the 'shared' mode in some cases for dynamic buffers.
// Investigate in which situations this could be beneficial.
static id<MTLBuffer> CreateDynamicBuffer(id<MTLDevice> device,
                                         GpuBufferType type,
                                         int maxUpdatesPerFrame,
                                         const void* data, unsigned size)
{
    MTLResourceOptions options =
        MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined;

    u32 roundedSize = RoundUp(size, s_bufferAlignments[type]);
    u32 padding = roundedSize - size;

    int numCopies = FRAMES_IN_FLIGHT * maxUpdatesPerFrame;
    NSUInteger physicalLength = size + (numCopies - 1) * roundedSize;

    id<MTLBuffer> buffer = [device newBufferWithLength:physicalLength
                                               options:options];

    if (data != NULL) {
        u8* contents = (u8*)[buffer contents];
        for (int i = 0; i < numCopies - 1; ++i) {
            memcpy(contents, data, size);
            memset(contents + size, 0, padding);
            contents += roundedSize;
        }
        memcpy(contents, data, size);
    }

    return buffer;
}

GpuBufferID GpuDeviceMetal::BufferCreate(GpuBufferType type,
                                         GpuBufferAccessMode accessMode,
                                         const void* data,
                                         unsigned size,
                                         int maxUpdatesPerFrame)
{
    ASSERT((accessMode != GPU_BUFFER_ACCESS_STREAM ||
           size <= STREAM_RING_BUF_SIZE)
           && "Maximum size of stream-mode buffer exceeded");
    ASSERT((accessMode != GPU_BUFFER_ACCESS_DYNAMIC ||
            maxUpdatesPerFrame > 0)
           && "Must have maxUpdatesPerFrame > 0 for dynamic-mode buffer");

    GpuBufferID bufferID(m_bufferTable.Add());
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
#ifdef GPUDEVICE_DEBUG_MODE
    buffer.dbg_refCount = 0;
#endif

    buffer.type = type;
    buffer.accessMode = accessMode;
    buffer.buffer = nil;
    buffer.maxUpdatesPerFrame = maxUpdatesPerFrame;
    buffer.size = size;
    buffer.bufOffset = 0;

    switch (accessMode) {
        case GPU_BUFFER_ACCESS_STATIC:
            buffer.buffer = CreateStaticBuffer(m_device, m_commandBuffer,
                                               data, size);
            break;
        case GPU_BUFFER_ACCESS_DYNAMIC:
            buffer.buffer = CreateDynamicBuffer(m_device, type,
                                                maxUpdatesPerFrame, data, size);
            break;
        case GPU_BUFFER_ACCESS_STREAM:
            break; // Don't need to do anything here.
        default:
            ASSERT(!"Unknown GpuBufferAccessMode");
            break;
    }

    ++m_dbg_bufferCount;

    return bufferID;
}

void GpuDeviceMetal::BufferStreamResize(GpuBufferID bufferID, unsigned newSize)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_STREAM);

    buffer.size = newSize;
}

void GpuDeviceMetal::BufferDestroy(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (buffer.dbg_refCount != 0) {
        FATAL("Can't destroy buffer as it still has %d draw "
              "item(s) referencing it", buffer.dbg_refCount);
    }
#endif

    [buffer.buffer release];

    m_bufferTable.Remove(bufferID);

    --m_dbg_bufferCount;
}

void* GpuDeviceMetal::DynamicBufferMap(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_DYNAMIC);

    u32 roundedSize = RoundUp(buffer.size, s_bufferAlignments[buffer.type]);

    int numCopies = FRAMES_IN_FLIGHT * buffer.maxUpdatesPerFrame;
    u32 physicalLength = buffer.size + (numCopies - 1) * roundedSize;

    buffer.bufOffset += roundedSize;
    if (buffer.bufOffset >= physicalLength)
        buffer.bufOffset = 0;

    void* result = (u8*)[buffer.buffer contents] + buffer.bufOffset;
    return result;
}

void GpuDeviceMetal::DynamicBufferUnmap(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_DYNAMIC);

    u32 roundedSize = RoundUp(buffer.size, s_bufferAlignments[buffer.type]);
    u32 padding = roundedSize - buffer.size;

    int numCopies = FRAMES_IN_FLIGHT * buffer.maxUpdatesPerFrame;
    u32 physicalLength = buffer.size + (numCopies - 1) * roundedSize;

    if (buffer.bufOffset + buffer.size < physicalLength) {
        // Write data to the 'padding' region of the buffer (used for alignment).
        // Writing the whole range of the buffer data may have performance
        // benefits if write combining is used.
        void* ptr = (u8*)[buffer.buffer contents] + buffer.bufOffset + buffer.size;
        memset(ptr, 0, padding);
    }
}

void* GpuDeviceMetal::StreamBufferMap(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_STREAM);

    // Check: we should always have an even number of ring buffers.
    ASSERT(m_streamRingBufs.size() % 2 == 0);
    int numRingBuffersPerFrame = (int)(m_streamRingBufs.size() / 2);

    // Round up the size so that subsequent offsets are aligned to a multiple
    // of the appropriate value.
    u32 roundedSize = RoundUp(buffer.size, s_bufferAlignments[buffer.type]);

    // Can the incoming batch fit in the current ring buffer?
    if (m_streamBufCursor + roundedSize > STREAM_RING_BUF_SIZE) {
        // Move to a new ring buffer and reset the cursor.
        ++m_streamBufIndex;
        m_streamBufCursor = 0;

        if (m_streamBufIndex == numRingBuffersPerFrame) {
            // Need to allocate a new ring buffer.
            // Actually, we allocate two ring buffers, one for each in-flight
            // frame.
            m_streamRingBufs.insert(m_streamRingBufs.begin() + numRingBuffersPerFrame,
                                    CreateStreamRingBuffer(m_device));
            m_streamRingBufs.push_back(CreateStreamRingBuffer(m_device));

            // Update the count.
            ++numRingBuffersPerFrame;
        }
    }

    int ringBufArrayIndex = numRingBuffersPerFrame * m_doubleBufferIndex
        + m_streamBufIndex;
    id<MTLBuffer> ringBuf = m_streamRingBufs[ringBufArrayIndex];

    void* result = (u8*)[ringBuf contents] + m_streamBufCursor;

    [buffer.buffer release];
    buffer.buffer = [ringBuf retain];
    buffer.bufOffset = m_streamBufCursor;
    m_streamBufCursor += roundedSize;

    return result;
}

void GpuDeviceMetal::StreamBufferUnmap(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_STREAM);
    // We don't actually need to do anything here.
}

void* GpuDeviceMetal::BufferMap(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_DYNAMIC ||
           buffer.accessMode == GPU_BUFFER_ACCESS_STREAM);
    if (buffer.accessMode == GPU_BUFFER_ACCESS_DYNAMIC)
        return DynamicBufferMap(bufferID);
    else if (buffer.accessMode == GPU_BUFFER_ACCESS_STREAM)
        return StreamBufferMap(bufferID);
    return NULL;
}

void GpuDeviceMetal::BufferUnmap(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_DYNAMIC ||
           buffer.accessMode == GPU_BUFFER_ACCESS_STREAM);
    if (buffer.accessMode == GPU_BUFFER_ACCESS_DYNAMIC)
        DynamicBufferUnmap(bufferID);
    else if (buffer.accessMode == GPU_BUFFER_ACCESS_STREAM)
        StreamBufferUnmap(bufferID);
}

bool GpuDeviceMetal::TextureExists(GpuTextureID textureID) const
{
    return m_textureTable.Has(textureID);
}

GpuTextureID GpuDeviceMetal::TextureCreate(GpuTextureType type,
                                           GpuPixelFormat pixelFormat,
                                           u32 flags,
                                           int width,
                                           int height,
                                           int depthOrArrayLength,
                                           int nMipmapLevels)
{
    ASSERT((s_textureTypeIsArray[type] ||
            (type == GPU_TEXTURE_3D) ||
            (depthOrArrayLength == 1)) &&
           "A non-3D, non-array texture must have depthOrArrayLength == 1");
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depthOrArrayLength > 0);
    ASSERT(nMipmapLevels > 0);

    GpuTextureID textureID(m_textureTable.Add());
    Texture& tex = m_textureTable.Lookup(textureID);
#ifdef GPUDEVICE_DEBUG_MODE
    tex.dbg_refCount = 0;
#endif

    MTLTextureDescriptor* desc = [[[MTLTextureDescriptor alloc] init] autorelease];
    desc.textureType = s_metalTextureTypes[type];
    desc.pixelFormat = s_metalPixelFormats[pixelFormat];
    desc.width = width;
    desc.height = height;
    desc.depth = (type == GPU_TEXTURE_3D) ? depthOrArrayLength : 1;
    desc.mipmapLevelCount = nMipmapLevels;
    desc.sampleCount = 1;
    desc.arrayLength = s_textureTypeIsArray[type] ? depthOrArrayLength : 1;
    desc.resourceOptions = MTLResourceStorageModeManaged | MTLResourceOptionCPUCacheModeDefault;
    desc.cpuCacheMode = MTLCPUCacheModeDefaultCache;

    if (flags & GPU_TEXTURE_FLAG_RENDER_TARGET) {
        desc.storageMode = MTLStorageModePrivate;
        desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    } else {
        desc.storageMode = MTLStorageModeManaged;
        desc.usage = MTLTextureUsageShaderRead;
    }

    tex.texture = [m_device newTextureWithDescriptor:desc];

    ++m_dbg_textureCount;

    return textureID;
}

void GpuDeviceMetal::TextureDestroy(GpuTextureID textureID)
{
    ASSERT(TextureExists(textureID));
    Texture& tex = m_textureTable.Lookup(textureID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (tex.dbg_refCount != 0) {
        FATAL("Can't destroy texture as it still has %d draw "
              "item(s) referencing it", tex.dbg_refCount);
    }
#endif

    [tex.texture release];

    m_textureTable.Remove(textureID);

    --m_dbg_textureCount;
}

void GpuDeviceMetal::TextureUpload(GpuTextureID textureID,
                                   const GpuRegion& region,
                                   int mipmapLevel,
                                   int stride,
                                   const void* bytes)
{
    ASSERT(TextureExists(textureID));
    Texture& tex = m_textureTable.Lookup(textureID);

    MTLRegion mtlRegion;
    mtlRegion.origin.x = region.x;
    mtlRegion.origin.y = region.y;
    mtlRegion.origin.z = 0;
    mtlRegion.size.width = region.width;
    mtlRegion.size.height = region.height;
    mtlRegion.size.depth = 1;

    [tex.texture replaceRegion:mtlRegion
                   mipmapLevel:mipmapLevel
                     withBytes:bytes
                   bytesPerRow:stride];
}

bool GpuDeviceMetal::SamplerExists(GpuSamplerID samplerID) const
{
    return m_samplerTable.Has(samplerID);
}

GpuSamplerID GpuDeviceMetal::SamplerCreate(const GpuSamplerDesc& desc)
{
    ASSERT(1 <= desc.maxAnisotropy && desc.maxAnisotropy <= 16);

    GpuSamplerID samplerID(m_samplerTable.Add());
    Sampler& sampler = m_samplerTable.Lookup(samplerID);
#ifdef GPUDEVICE_DEBUG_MODE
    sampler.dbg_refCount = 0;
#endif

    MTLSamplerDescriptor* mtlDesc = [[[MTLSamplerDescriptor alloc] init] autorelease];
    mtlDesc.sAddressMode = s_metalSamplerAddressModes[desc.uAddressMode];
    mtlDesc.tAddressMode = s_metalSamplerAddressModes[desc.vAddressMode];
    mtlDesc.rAddressMode = s_metalSamplerAddressModes[desc.wAddressMode];
    mtlDesc.minFilter = s_metalSamplerMinMagFilters[desc.minFilter];
    mtlDesc.magFilter = s_metalSamplerMinMagFilters[desc.magFilter];
    mtlDesc.mipFilter = s_metalSamplerMipFilters[desc.mipFilter];
    mtlDesc.lodMinClamp = 0.0f;
    mtlDesc.lodMaxClamp = FLT_MAX;
    mtlDesc.maxAnisotropy = desc.maxAnisotropy;
    mtlDesc.normalizedCoordinates = YES;
    mtlDesc.compareFunction = MTLCompareFunctionNever;

    sampler.samplerState = [m_device newSamplerStateWithDescriptor:mtlDesc];

    ++m_dbg_samplerCount;

    return samplerID;
}

void GpuDeviceMetal::SamplerDestroy(GpuSamplerID samplerID)
{
    ASSERT(SamplerExists(samplerID));
    Sampler& sampler = m_samplerTable.Lookup(samplerID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (sampler.dbg_refCount != 0) {
        FATAL("Can't destroy sampler as it still has %d draw "
              "item(s) referencing it", sampler.dbg_refCount);
    }
#endif

    [sampler.samplerState release];

    m_samplerTable.Remove(samplerID);

    --m_dbg_samplerCount;
}

bool GpuDeviceMetal::InputLayoutExists(GpuInputLayoutID inputLayoutID) const
{
    return m_inputLayoutTable.Has(inputLayoutID);
}

GpuInputLayoutID GpuDeviceMetal::InputLayoutCreate(int nVertexAttribs,
                                                   const GpuVertexAttribute* attribs,
                                                   int nVertexBuffers,
                                                   const unsigned* strides)
{
    GpuInputLayoutID inputLayoutID(m_inputLayoutTable.Add());
    InputLayout& layout = m_inputLayoutTable.Lookup(inputLayoutID);
#ifdef GPUDEVICE_DEBUG_MODE
    layout.dbg_refCount = 0;
#endif

    layout.descriptor = [[MTLVertexDescriptor alloc] init];

    for (int i = 0; i < nVertexAttribs; ++i) {
        MTLVertexAttributeDescriptor* attribDesc;
        attribDesc = [[[MTLVertexAttributeDescriptor alloc] init] autorelease];
        attribDesc.format = s_metalVertexAttribFormats[attribs[i].format];
        attribDesc.offset = attribs[i].offset;
        attribDesc.bufferIndex = GPU_MAX_CBUFFERS + attribs[i].bufferSlot;
        [layout.descriptor.attributes setObject:attribDesc
                             atIndexedSubscript:i];
    }

    for (int i = 0; i < nVertexBuffers; ++i) {
        MTLVertexBufferLayoutDescriptor* layoutDesc;
        layoutDesc = [[[MTLVertexBufferLayoutDescriptor alloc] init] autorelease];
        layoutDesc.stride = strides[i];
        [layout.descriptor.layouts setObject:layoutDesc
                          atIndexedSubscript:(GPU_MAX_CBUFFERS + i)];
    }

    ++m_dbg_inputLayoutCount;

    return inputLayoutID;
}

void GpuDeviceMetal::InputLayoutDestroy(GpuInputLayoutID inputLayoutID)
{
    ASSERT(InputLayoutExists(inputLayoutID));
    InputLayout& layout = m_inputLayoutTable.Lookup(inputLayoutID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (layout.dbg_refCount != 0) {
        FATAL("Can't destroy input layout as it still has %d pipeline state "
              "object(s) referencing it", layout.dbg_refCount);
    }
    [layout.descriptor release];
#endif

    m_inputLayoutTable.Remove(inputLayoutID);

    --m_dbg_inputLayoutCount;
}

bool GpuDeviceMetal::PipelineStateExists(GpuPipelineStateID pipelineStateID) const
{
    return m_pipelineStateTable.Has(pipelineStateID);
}

GpuPipelineStateID GpuDeviceMetal::PipelineStateCreate(const GpuPipelineStateDesc& state)
{
    ASSERT(ShaderProgramExists(state.shaderProgram));
    ASSERT(InputLayoutExists(state.inputLayout));

    GpuPipelineStateID pipelineStateID(m_pipelineStateTable.Add());
    PipelineStateObj& obj = m_pipelineStateTable.Lookup(pipelineStateID);
#ifdef GPUDEVICE_DEBUG_MODE
    obj.dbg_refCount = 0;
    obj.dbg_shaderProgram = state.shaderProgram;
    obj.dbg_inputLayout = state.inputLayout;
    ++m_shaderProgramTable.Lookup(state.shaderProgram).dbg_refCount;
    ++m_inputLayoutTable.Lookup(state.inputLayout).dbg_refCount;
#endif

    obj.state = CreateMTLRenderPipelineState(state);
    obj.depthStencilState = CreateMTLDepthStencilState(state);
    obj.triangleFillMode = s_metalFillModes[state.fillMode];
    obj.cullMode = s_metalCullModes[state.cullMode];
    obj.frontFaceWinding = s_metalWindingOrders[state.frontFaceWinding];

    ++m_dbg_psoCount;

    return pipelineStateID;
}

id<MTLRenderPipelineState> GpuDeviceMetal::CreateMTLRenderPipelineState(const GpuPipelineStateDesc& state)
{
    ShaderProgram& shaderProgram = m_shaderProgramTable.Lookup(state.shaderProgram);
    u32 idxPermutation = m_permutations.FindPermutationForStates(
        shaderProgram.idxFirstPermutation,
        state.shaderStateBitfield
    );
    PermutationApiData& permutation = m_permutations.Lookup(idxPermutation).program;

    MTLRenderPipelineDescriptor* desc = [[[MTLRenderPipelineDescriptor alloc] init] autorelease];

    // Set the shaders and input layout
    desc.vertexFunction = permutation.vertexFunction;
    desc.fragmentFunction = permutation.fragmentFunction;
    desc.vertexDescriptor = m_inputLayoutTable.Lookup(state.inputLayout).descriptor;

    MTLRenderPipelineColorAttachmentDescriptor* colorDesc;
    colorDesc = [[[MTLRenderPipelineColorAttachmentDescriptor alloc] init] autorelease];

    colorDesc.pixelFormat = s_metalColorPixelFormats[m_deviceFormat.pixelColorFormat];

    // Setup blending
    colorDesc.blendingEnabled = state.blendingEnabled ? YES : NO;
    colorDesc.rgbBlendOperation = MTLBlendOperationAdd;
    colorDesc.alphaBlendOperation = MTLBlendOperationAdd;
    colorDesc.sourceRGBBlendFactor = s_metalBlendFactors[state.blendSrcFactor];
    colorDesc.sourceAlphaBlendFactor = s_metalBlendFactors[state.blendSrcFactor];
    colorDesc.destinationRGBBlendFactor = s_metalBlendFactors[state.blendDstFactor];
    colorDesc.destinationAlphaBlendFactor = s_metalBlendFactors[state.blendDstFactor];

    [desc.colorAttachments setObject:colorDesc atIndexedSubscript:0];

    desc.depthAttachmentPixelFormat = s_metalDepthPixelFormats[m_deviceFormat.pixelDepthFormat];

    NSError* error;
    id<MTLRenderPipelineState> result;
    result = [m_device newRenderPipelineStateWithDescriptor:desc error:&error];
    if (error) {
        FATAL("GpuDeviceMetal: Failed to create pipeline object: %s",
              error.localizedDescription.UTF8String);
    }
    return result;
}

id<MTLDepthStencilState> GpuDeviceMetal::CreateMTLDepthStencilState(const GpuPipelineStateDesc& state)
{
    MTLDepthStencilDescriptor* desc = [[[MTLDepthStencilDescriptor alloc] init] autorelease];
    desc.depthCompareFunction = s_metalCompareFunctions[state.depthCompare];
    desc.depthWriteEnabled = state.depthWritesEnabled ? YES : NO;

    return [m_device newDepthStencilStateWithDescriptor:desc];
}

void GpuDeviceMetal::PipelineStateDestroy(GpuPipelineStateID pipelineStateID)
{
    ASSERT(PipelineStateExists(pipelineStateID));
    PipelineStateObj& obj = m_pipelineStateTable.Lookup(pipelineStateID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (obj.dbg_refCount != 0) {
        FATAL("Can't destroy pipeline state object as it still has %d draw "
              "item(s) referencing it", obj.dbg_refCount);
    }
    --m_shaderProgramTable.Lookup(obj.dbg_shaderProgram).dbg_refCount;
    --m_inputLayoutTable.Lookup(obj.dbg_inputLayout).dbg_refCount;
#endif

    [obj.state release];
    [obj.depthStencilState release];
    m_pipelineStateTable.Remove(pipelineStateID);

    --m_dbg_psoCount;
}

bool GpuDeviceMetal::RenderPassExists(GpuRenderPassID renderPassID) const
{
    return m_renderPassTable.Has(renderPassID);
}

static MTLLoadAction GetMTLColorLoadAction(const GpuRenderPassDesc& desc, int index)
{
    if (desc.colorLoadActions)
        return s_metalLoadActions[desc.colorLoadActions[index]];
    return s_metalLoadActions[GpuRenderPassDesc::DEFAULT_LOAD_ACTION];
}

static MTLStoreAction GetMTLColorStoreAction(const GpuRenderPassDesc& desc, int index)
{
    if (desc.colorStoreActions)
        return s_metalStoreActions[desc.colorStoreActions[index]];
    return s_metalStoreActions[GpuRenderPassDesc::DEFAULT_STORE_ACTION];
}

static MTLClearColor GetMTLClearColor(const GpuRenderPassDesc& desc, int index)
{
    if (desc.clearColors) {
        GpuColor color = desc.clearColors[index];
        return MTLClearColorMake(
            (double)color.r,
            (double)color.g,
            (double)color.b,
            (double)color.a
        );
    }
    return MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
}

GpuRenderPassID GpuDeviceMetal::RenderPassCreate(const GpuRenderPassDesc& pass)
{
    ASSERT(pass.numRenderTargets >= 0);

    GpuRenderPassID renderPassID(m_renderPassTable.Add());
    RenderPassObj& obj = m_renderPassTable.Lookup(renderPassID);
    obj.usesRenderTarget = false;

    obj.descriptor = [[MTLRenderPassDescriptor alloc] init];

    if (pass.numRenderTargets > 0) {
        ASSERT(pass.renderTargets != NULL);
        obj.usesRenderTarget = true;
        for (int i = 0; i < pass.numRenderTargets; ++i) {
            ASSERT(pass.renderTargets[i] != 0);

            MTLRenderPassColorAttachmentDescriptor* desc;
            desc = [[[MTLRenderPassColorAttachmentDescriptor alloc] init] autorelease];

            desc.clearColor = GetMTLClearColor(pass, i);
            desc.loadAction = GetMTLColorLoadAction(pass, i);
            desc.storeAction = GetMTLColorStoreAction(pass, i);
            desc.texture = m_textureTable.Lookup(pass.renderTargets[i]).texture;

            [obj.descriptor.colorAttachments setObject:desc
                                    atIndexedSubscript:(NSUInteger)i];
        }
    } else {
        MTLRenderPassColorAttachmentDescriptor* desc;
        desc = [[[MTLRenderPassColorAttachmentDescriptor alloc] init] autorelease];

        desc.clearColor = GetMTLClearColor(pass, 0);
        desc.loadAction = GetMTLColorLoadAction(pass, 0);
        desc.storeAction = GetMTLColorStoreAction(pass, 0);

        [obj.descriptor.colorAttachments setObject:desc atIndexedSubscript:0];
    }

    MTLRenderPassDepthAttachmentDescriptor* depthDesc = nil;

    if (m_deviceFormat.pixelDepthFormat != GPU_PIXEL_DEPTH_FORMAT_NONE
        || pass.depthStencilTarget != 0) {
        depthDesc = [[[MTLRenderPassDepthAttachmentDescriptor alloc] init] autorelease];
        depthDesc.clearDepth = pass.clearDepth;
        depthDesc.loadAction = s_metalLoadActions[pass.depthStencilLoadAction];
        depthDesc.storeAction = s_metalStoreActions[pass.depthStencilStoreAction];

        if (pass.depthStencilTarget != 0)
            depthDesc.texture = m_textureTable.Lookup(pass.depthStencilTarget).texture;
    }

    obj.descriptor.depthAttachment = depthDesc;

    ++m_dbg_renderPassCount;

    return renderPassID;
}

void GpuDeviceMetal::RenderPassDestroy(GpuRenderPassID renderPassID)
{
    ASSERT(RenderPassExists(renderPassID));
    RenderPassObj& obj = m_renderPassTable.Lookup(renderPassID);
    [obj.descriptor release];
    m_renderPassTable.Remove(renderPassID);

    --m_dbg_renderPassCount;
}

static MTLViewport GetMTLViewport(const GpuViewport& viewport)
{
    MTLViewport res;
    res.originX = (double)viewport.x;
    res.originY = (double)viewport.y;
    res.width = (double)viewport.width;
    res.height = (double)viewport.height;
    res.znear = (double)viewport.zNear;
    res.zfar = (double)viewport.zFar;
    return res;
}

id<MTLRenderCommandEncoder> GpuDeviceMetal::PreDraw(GpuRenderPassID passID,
                                                    const GpuViewport& viewport)
{
    RenderPassObj& pass = m_renderPassTable.Lookup(passID);

    if (!pass.usesRenderTarget) {
        // Set up the render pass to write to the backbuffer.
        MTLRenderPassColorAttachmentDescriptor* colorDesc;
        colorDesc = [pass.descriptor.colorAttachments objectAtIndexedSubscript:0];
        colorDesc.texture = m_currentDrawable.texture;
        ASSERT(colorDesc.texture.width == m_deviceFormat.resolutionX);
        ASSERT(colorDesc.texture.height == m_deviceFormat.resolutionY);

        id<MTLTexture> depthBuf = m_depthBufs[m_doubleBufferIndex];
        pass.descriptor.depthAttachment.texture = depthBuf;
        ASSERT(depthBuf.width == m_deviceFormat.resolutionX);
        ASSERT(depthBuf.height == m_deviceFormat.resolutionY);
    }

    id<MTLRenderCommandEncoder> encoder;
    encoder = [m_commandBuffer renderCommandEncoderWithDescriptor:pass.descriptor];
    [encoder setViewport:GetMTLViewport(viewport)];
    return encoder;
}

void GpuDeviceMetal::PostDraw(id<MTLRenderCommandEncoder> encoder)
{
    [encoder endEncoding];
}

void GpuDeviceMetal::Draw(const GpuDrawItem* const* items,
                          int nItems,
                          GpuRenderPassID renderPass,
                          const GpuViewport& viewport)
{
    ASSERT(items != NULL);
    ASSERT(RenderPassExists(renderPass));

    id<MTLRenderCommandEncoder> encoder = PreDraw(renderPass, viewport);

    for (int drawItemIndex = 0; drawItemIndex < nItems; ++drawItemIndex) {
        const GpuDrawItem* item = items[drawItemIndex];
        ASSERT(item != NULL);

        // Set the pipeline state
        PipelineStateObj& pipelineState
            = m_pipelineStateTable.LookupRaw(item->pipelineStateIdx);
        [encoder setRenderPipelineState:pipelineState.state];
        [encoder setDepthStencilState:pipelineState.depthStencilState];
        [encoder setTriangleFillMode:pipelineState.triangleFillMode];
        [encoder setCullMode:pipelineState.cullMode];
        [encoder setFrontFacingWinding:pipelineState.frontFaceWinding];

        // Set vertex buffers
        const u32* vertexBufferOffsets = item->VertexBufferOffsets();
        const u16* vertexBuffers = item->VertexBuffers();
        for (int i = 0; i < item->nVertexBuffers; ++i) {
            Buffer& buf = m_bufferTable.LookupRaw(vertexBuffers[i]);
            u32 totalOffset = vertexBufferOffsets[i] + buf.bufOffset;
            [encoder setVertexBuffer:buf.buffer
                              offset:totalOffset
                             atIndex:(GPU_MAX_CBUFFERS + i)];
        }

        // Set cbuffers
        const u16* cbuffers = item->CBuffers();
        for (int i = 0; i < item->nCBuffers; ++i) {
            Buffer& buf = m_bufferTable.LookupRaw(cbuffers[i]);
            [encoder setVertexBuffer:buf.buffer
                              offset:buf.bufOffset
                             atIndex:i];
            [encoder setFragmentBuffer:buf.buffer
                                offset:buf.bufOffset
                               atIndex:i];
        }

        // Set textures
        const u16* textures = item->Textures();
        for (int i = 0; i < item->nTextures; ++i) {
            id<MTLTexture> tex = m_textureTable.LookupRaw(textures[i]).texture;
            [encoder setVertexTexture:tex atIndex:i];
            [encoder setFragmentTexture:tex atIndex:i];
        }

        // Set samplers
        const u16* samplers = item->Samplers();
        for (int i = 0; i < item->nSamplers; ++i) {
            id<MTLSamplerState> sampler = m_samplerTable.LookupRaw(samplers[i]).samplerState;
            [encoder setVertexSamplerState:sampler atIndex:i];
            [encoder setFragmentSamplerState:sampler atIndex:i];
        }

        // Submit the draw call
        MTLPrimitiveType primType = s_metalPrimitiveTypes[item->GetPrimitiveType()];
        if (item->IsIndexed()) {
            GpuIndexType gpuIndexType = item->GetIndexType();
            id<MTLBuffer> indexBuf = m_bufferTable.LookupRaw(item->indexBufferIdx).buffer;
            NSUInteger indexBufOffset = item->indexBufferOffset;
            indexBufOffset += item->first * s_indexTypeByteSizes[gpuIndexType];
            [encoder drawIndexedPrimitives:primType
                                indexCount:item->count
                                 indexType:s_metalIndexTypes[gpuIndexType]
                               indexBuffer:indexBuf
                         indexBufferOffset:indexBufOffset];
        } else {
            [encoder drawPrimitives:primType
                        vertexStart:item->first
                        vertexCount:item->count];
        }
    }

    PostDraw(encoder);
}

void GpuDeviceMetal::SceneBegin()
{
    @autoreleasepool {
        [m_currentDrawable release];
        m_currentDrawable = [[GetCAMetalLayer() nextDrawable] retain];
        ASSERT(m_currentDrawable);

        // Switch to the other frame buffer.
        m_doubleBufferIndex ^= 1;

        // Reset the stream buffer index.
        m_streamBufIndex = 0;
        m_streamBufCursor = 0;
    }
}

void GpuDeviceMetal::ScenePresent()
{
    [m_commandBuffer presentDrawable:m_currentDrawable];
    [m_commandBuffer commit];

    [m_prevCommandBuffer waitUntilCompleted];
    [m_prevCommandBuffer release];

    m_prevCommandBuffer = m_commandBuffer;

    m_commandBuffer = [[m_commandQueue commandBuffer] retain];

    ++m_frameNumber;
}

#ifdef GPUDEVICE_DEBUG_MODE
void GpuDeviceMetal::DrawItem_UpdateRefCounts(const GpuDrawItem* item, int increment)
{
    m_pipelineStateTable.LookupRaw(item->pipelineStateIdx).dbg_refCount += increment;

    if (item->IsIndexed())
        m_bufferTable.LookupRaw(item->indexBufferIdx).dbg_refCount += increment;

    u16* vertexBuffers = item->VertexBuffers();
    for (int i = 0; i < item->nVertexBuffers; ++i) {
        m_bufferTable.LookupRaw(vertexBuffers[i]).dbg_refCount += increment;
    }

    u16* cbuffers = item->CBuffers();
    for (int i = 0; i < item->nCBuffers; ++i) {
        m_bufferTable.LookupRaw(cbuffers[i]).dbg_refCount += increment;
    }

    u16* textures = item->Textures();
    for (int i = 0; i < item->nTextures; ++i) {
        m_textureTable.LookupRaw(textures[i]).dbg_refCount += increment;
    }

    u16* samplers = item->Samplers();
    for (int i = 0; i < item->nSamplers; ++i) {
        m_samplerTable.LookupRaw(samplers[i]).dbg_refCount += increment;
    }
}

void GpuDeviceMetal::RegisterDrawItem(const GpuDrawItem* item)
{
    DrawItem_UpdateRefCounts(item, 1);
}

void GpuDeviceMetal::UnregisterDrawItem(const GpuDrawItem* item)
{
    DrawItem_UpdateRefCounts(item, -1);
}
#endif // GPUDEVICE_DEBUG_MODE

static const GpuDeviceMetal* Cast(const GpuDevice* dev) { return (const GpuDeviceMetal*)dev; }
static GpuDeviceMetal* Cast(GpuDevice* dev) { return (GpuDeviceMetal*)dev; }
static GpuDevice* Cast(GpuDeviceMetal* dev) { return (GpuDevice*)dev; }

GpuDevice* GpuDevice::Create(const GpuDeviceFormat& format, void* osViewHandle)
{ return Cast(new GpuDeviceMetal(format, osViewHandle)); }

void GpuDevice::Destroy(GpuDevice* dev) { delete Cast(dev); }

void GpuDevice::SetFormat(const GpuDeviceFormat& format)
{ Cast(this)->SetFormat(format); }

const GpuDeviceFormat& GpuDevice::GetFormat() const
{ return Cast(this)->GetFormat(); }

void GpuDevice::OnWindowResized()
{ Cast(this)->OnWindowResized(); }

bool GpuDevice::ShaderProgramExists(GpuShaderProgramID shaderProgramID) const
{ return Cast(this)->ShaderProgramExists(shaderProgramID); }

GpuShaderProgramID GpuDevice::ShaderProgramCreate(const char* data, size_t length)
{ return Cast(this)->ShaderProgramCreate(data, length); }

void GpuDevice::ShaderProgramDestroy(GpuShaderProgramID shaderProgramID)
{ Cast(this)->ShaderProgramDestroy(shaderProgramID); }

bool GpuDevice::BufferExists(GpuBufferID bufferID) const
{ return Cast(this)->BufferExists(bufferID); }

GpuBufferID GpuDevice::BufferCreate(GpuBufferType type,
                                    GpuBufferAccessMode accessMode,
                                    const void* data,
                                    unsigned size,
                                    int maxUpdatesPerFrame)
{ return Cast(this)->BufferCreate(type, accessMode, data, size, maxUpdatesPerFrame); }

void GpuDevice::BufferDestroy(GpuBufferID bufferID)
{ Cast(this)->BufferDestroy(bufferID); }

void GpuDevice::BufferStreamResize(GpuBufferID bufferID, unsigned newSize)
{ Cast(this)->BufferStreamResize(bufferID, newSize); }

void* GpuDevice::BufferMap(GpuBufferID bufferID)
{ return Cast(this)->BufferMap(bufferID); }

void GpuDevice::BufferUnmap(GpuBufferID bufferID)
{ Cast(this)->BufferUnmap(bufferID); }

bool GpuDevice::TextureExists(GpuTextureID textureID) const
{ return Cast(this)->TextureExists(textureID); }

GpuTextureID GpuDevice::TextureCreate(GpuTextureType type,
                                      GpuPixelFormat pixelFormat,
                                      u32 flags,
                                      int width,
                                      int height,
                                      int depthOrArrayLength,
                                      int nMipmapLevels)
{
    return Cast(this)->TextureCreate(
        type,
        pixelFormat,
        flags,
        width,
        height,
        depthOrArrayLength,
        nMipmapLevels
    );
}

void GpuDevice::TextureDestroy(GpuTextureID textureID)
{ return Cast(this)->TextureDestroy(textureID); }

void GpuDevice::TextureUpload(GpuTextureID textureID,
                              const GpuRegion& region,
                              int mipmapLevel,
                              int stride,
                              const void* bytes)
{ Cast(this)->TextureUpload(textureID, region, mipmapLevel, stride, bytes); }

bool GpuDevice::SamplerExists(GpuSamplerID samplerID) const
{ return Cast(this)->SamplerExists(samplerID); }

GpuSamplerID GpuDevice::SamplerCreate(const GpuSamplerDesc& desc)
{ return Cast(this)->SamplerCreate(desc); }

void GpuDevice::SamplerDestroy(GpuSamplerID samplerID)
{ Cast(this)->SamplerDestroy(samplerID); }

bool GpuDevice::InputLayoutExists(GpuInputLayoutID inputLayoutID) const
{ return Cast(this)->InputLayoutExists(inputLayoutID); }

GpuInputLayoutID GpuDevice::InputLayoutCreate(int nVertexAttribs,
                                              const GpuVertexAttribute* attribs,
                                              int nVertexBuffers,
                                              const unsigned* strides)
{ return Cast(this)->InputLayoutCreate(nVertexAttribs, attribs, nVertexBuffers, strides); }

void GpuDevice::InputLayoutDestroy(GpuInputLayoutID inputLayoutID)
{ Cast(this)->InputLayoutDestroy(inputLayoutID); }

bool GpuDevice::PipelineStateExists(GpuPipelineStateID pipelineStateID) const
{ return Cast(this)->PipelineStateExists(pipelineStateID); }

GpuPipelineStateID GpuDevice::PipelineStateCreate(const GpuPipelineStateDesc& state)
{ return Cast(this)->PipelineStateCreate(state); }

void GpuDevice::PipelineStateDestroy(GpuPipelineStateID pipelineStateID)
{ Cast(this)->PipelineStateDestroy(pipelineStateID); }

bool GpuDevice::RenderPassExists(GpuRenderPassID renderPassID) const
{ return Cast(this)->RenderPassExists(renderPassID); }

GpuRenderPassID GpuDevice::RenderPassCreate(const GpuRenderPassDesc& pass)
{ return Cast(this)->RenderPassCreate(pass); }

void GpuDevice::RenderPassDestroy(GpuRenderPassID renderPassID)
{ Cast(this)->RenderPassDestroy(renderPassID); }

void GpuDevice::Draw(const GpuDrawItem* const* items,
                     int nItems,
                     GpuRenderPassID renderPass,
                     const GpuViewport& viewport)
{ Cast(this)->Draw(items, nItems, renderPass, viewport); }

void GpuDevice::SceneBegin()
{ Cast(this)->SceneBegin(); }

void GpuDevice::ScenePresent()
{ Cast(this)->ScenePresent(); }

#ifdef GPUDEVICE_DEBUG_MODE
void GpuDevice::RegisterDrawItem(const GpuDrawItem* item)
{ Cast(this)->RegisterDrawItem(item); }

void GpuDevice::UnregisterDrawItem(const GpuDrawItem* item)
{ Cast(this)->UnregisterDrawItem(item); }
#endif // GPUDEVICE_DEBUG_MODE

Matrix44 GpuDevice::TransformCreateOrtho(float left, float right,
                                         float bot, float top,
                                         float near, float far)
{
    float tx = -(right + left) / (right - left);
    float ty = -(top + bot) / (top - bot);
    float tz = -(near) / (far - near);
    return Matrix44(2.0f / (right - left), 0.0f, 0.0f, tx,
                    0.0f, 2.0f / (top - bot), 0.0f, ty,
                    0.0f, 0.0f, -1.0f / (far - near), tz,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix44 GpuDevice::TransformCreatePerspective(float left, float right,
                                               float bot, float top,
                                               float near, float far)
{
    float A = (right + left) / (right - left);
    float B = (top + bot) / (top - bot);
    float C = -(far) / (far - near);
    float D = -(far * near) / (far - near);
    return Matrix44(2.0f * near / (right - left), 0.0f, A, 0.0f,
                    0.0f, 2.0f * near / (top - bot), B, 0.0f,
                    0.0f, 0.0f, C, D,
                    0.0f, 0.0f, -1.0f, 0.0f);
}

#endif // GPUDEVICE_API_METAL
