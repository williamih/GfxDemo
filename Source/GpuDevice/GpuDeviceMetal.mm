#include "GpuDevice/GpuDevice.h"

#include <stdio.h>

#import <Cocoa/Cocoa.h> // for NSView
#import <QuartzCore/QuartzCore.h> // for CAMetalLayer
#import <Metal/Metal.h>

#include "IDLookupTable.h"
#include "Core/Macros.h"
#include "GpuDevice/GpuDrawItem.h"

// -----------------------------------------------------------------------------
// Lookup tables
// -----------------------------------------------------------------------------

static const MTLPixelFormat s_metalColorPixelFormats[] = {
    MTLPixelFormatBGRA8Unorm, // GPUPIXELCOLORFORMAT_RGBA8888
};

// Metal doesn't support a 24-bit depth buffer. We use 32-bit no matter what
// the client requests.
static const MTLPixelFormat s_metalDepthPixelFormats[] = {
    MTLPixelFormatInvalid, // GPUPIXELDEPTHFORMAT_NONE
    MTLPixelFormatDepth32Float, // GPUPIXELDEPTHFORMAT_FLOAT24
    MTLPixelFormatDepth32Float, // GPUPIXELDEPTHFORMAT_FLOAT32
};

static NSString* const s_shaderEntryPointNames[] = {
    @"VertexMain", // GPUSHADERTYPE_VERTEX
    @"PixelMain", // GPUSHADERTYPE_PIXEL
};

static MTLResourceOptions s_bufferAccessModeToResourceOptions[] = {
    // GPUBUFFER_ACCESS_STATIC
    MTLResourceStorageModeManaged | MTLResourceCPUCacheModeDefaultCache,

    // GPUBUFFER_ACCESS_DYNAMIC
    MTLResourceStorageModeManaged | MTLResourceCPUCacheModeWriteCombined,
};

static const MTLPrimitiveType s_metalPrimitiveTypes[] = {
    MTLPrimitiveTypeTriangle, // GPUPRIMITIVE_TRIANGLES 
};

static const MTLIndexType s_metalIndexTypes[] = {
    MTLIndexTypeUInt16, // GPUINDEXTYPE_U16
    MTLIndexTypeUInt32, // GPUINDEXTYPE_U32
};

static const NSUInteger s_indexTypeByteSizes[] = {
    2, // GPUINDEXTYPE_U16
    4, // GPUINDEXTYPE_U32
};

static const MTLVertexFormat s_metalVertexAttribFormats[] = {
    MTLVertexFormatHalf2, // GPUVERTEXATTRIB_HALF2
    MTLVertexFormatHalf3, // GPUVERTEXATTRIB_HALF3
    MTLVertexFormatHalf4, // GPUVERTEXATTRIB_HALF4
    MTLVertexFormatFloat, // GPUVERTEXATTRIB_FLOAT
    MTLVertexFormatFloat2, // GPUVERTEXATTRIB_FLOAT2
    MTLVertexFormatFloat3, // GPUVERTEXATTRIB_FLOAT3
    MTLVertexFormatFloat4, // GPUVERTEXATTRIB_FLOAT4
    MTLVertexFormatUChar4Normalized, // GPUVERTEXATTRIB_UBYTE4_NORMALIZED
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

// -----------------------------------------------------------------------------
// GpuDeviceMetal class declaration
// -----------------------------------------------------------------------------

class GpuDeviceMetal {
public:
    GpuDeviceMetal(const GpuDeviceFormat& format, void* osViewHandle);
    ~GpuDeviceMetal();

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

    GpuPipelineStateID CreatePipelineStateObject(const GpuPipelineStateDesc& state);
private:
    id<MTLRenderPipelineState> CreateMTLRenderPipelineState(const GpuPipelineStateDesc& state);
    id<MTLDepthStencilState> CreateMTLDepthStencilState(const GpuPipelineStateDesc& state);

public:
    GpuRenderPassID CreateRenderPassObject(const GpuRenderPassDesc& pass);
    GpuInputLayoutID CreateInputLayout(int nVertexAttribs,
                                       const GpuVertexAttribute* attribs,
                                       int nVertexBuffers,
                                       const unsigned* strides);

private:
    id<MTLRenderCommandEncoder> PreDraw(GpuRenderPassID passID, const GpuViewport& viewport);
    void PostDraw(id<MTLRenderCommandEncoder> encoder);
public:
    void Draw(const GpuDrawItem* const* items,
              int nItems,
              GpuRenderPassID renderPass,
              const GpuViewport& viewport);

    void SceneBegin();
    void ScenePresent();
private:
    GpuDeviceMetal(const GpuDeviceMetal&);
    GpuDeviceMetal& operator=(const GpuDeviceMetal&);

    struct Shader {
        id<MTLLibrary> library;
        id<MTLFunction> function;
    };

    struct Buffer {
        id<MTLBuffer> buffer;
        GpuBufferType type;
        GpuBufferAccessMode accessMode;
    };

    struct PipelineStateObj {
        id<MTLRenderPipelineState> state;
        id<MTLDepthStencilState> depthStencilState;
    };

    struct RenderPassObj {
        MTLRenderPassDescriptor* descriptor;
    };

    struct InputLayout {
        MTLVertexDescriptor* descriptor;
    };

    void CreateOrDestroyDepthBuffer();
    CAMetalLayer* GetCAMetalLayer() const;

    GpuDeviceFormat m_deviceFormat;
    NSView* m_view;
    CGSize m_viewSize;
    int m_frameNumber;

    id<MTLDevice> m_device;
    id<MTLTexture> m_depthBuf;

    id<MTLCommandQueue> m_commandQueue;
    id<MTLCommandBuffer> m_commandBuffer;

    id<CAMetalDrawable> m_currentDrawable;

    IDLookupTable<Shader, GpuShaderID::Type, 16, 16> m_shaderTable;
    IDLookupTable<Buffer, GpuBufferID::Type, 16, 16> m_bufferTable;
    IDLookupTable<PipelineStateObj, GpuPipelineStateID::Type, 16, 16> m_pipelineStateTable;
    IDLookupTable<RenderPassObj, GpuRenderPassID::Type, 16, 16> m_renderPassTable;
    IDLookupTable<InputLayout, GpuInputLayoutID::Type, 16, 16> m_inputLayoutTable;
};

// -----------------------------------------------------------------------------
// GpuDeviceMetal implementation
// -----------------------------------------------------------------------------

GpuDeviceMetal::GpuDeviceMetal(const GpuDeviceFormat& format, void* osViewHandle)
    : m_deviceFormat(format)
    , m_view((NSView*)osViewHandle)
    , m_viewSize([m_view bounds].size)
    , m_frameNumber(0)

    , m_device(nil)
    , m_depthBuf(nil)

    , m_commandQueue(nil)
    , m_commandBuffer(nil)

    , m_currentDrawable(nil)

    , m_shaderTable()
    , m_bufferTable()
    , m_pipelineStateTable()
    , m_renderPassTable()
    , m_inputLayoutTable()
{
    if (![[m_view layer] isKindOfClass:[CAMetalLayer class]])
        FATAL("GpuDeviceMetal: View needs to have a CAMetalLayer");

    m_device = MTLCreateSystemDefaultDevice();
    GetCAMetalLayer().device = m_device;
    GetCAMetalLayer().drawableSize = CGSizeMake(format.resolutionX, format.resolutionY);
    GetCAMetalLayer().pixelFormat = s_metalColorPixelFormats[format.pixelColorFormat];
    CreateOrDestroyDepthBuffer();

    m_commandQueue = [m_device newCommandQueue];
}

GpuDeviceMetal::~GpuDeviceMetal()
{
    [m_commandQueue release];
    [m_device release];
}

void GpuDeviceMetal::CreateOrDestroyDepthBuffer()
{
    // Destroy the current depth buffer, if one already exists
    if (m_depthBuf) {
        [m_depthBuf release];
        m_depthBuf = nil;
    }

    // If the client didn't request a depth buffer, then we are done.
    if (m_deviceFormat.pixelDepthFormat == GPUPIXELDEPTHFORMAT_NONE)
        return;

    MTLPixelFormat format = s_metalDepthPixelFormats[m_deviceFormat.pixelDepthFormat];

    MTLTextureDescriptor* desc;
    desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
                                                              width:m_deviceFormat.resolutionX
                                                             height:m_deviceFormat.resolutionY
                                                          mipmapped:NO];
    desc.storageMode = MTLStorageModePrivate;
    desc.usage |= MTLTextureUsageRenderTarget;
    m_depthBuf = [m_device newTextureWithDescriptor:desc];
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
    CreateOrDestroyDepthBuffer();
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

    CreateOrDestroyDepthBuffer();
}

GpuShaderID GpuDeviceMetal::CreateShader(GpuShaderType type, const char* code, size_t length)
{
    GpuShaderID shaderID(m_shaderTable.Add());
    Shader& shader = m_shaderTable.Lookup(shaderID);

    void* codeCopy = malloc(length);
    memcpy(codeCopy, code, length);
    dispatch_data_t data = dispatch_data_create(codeCopy, length, NULL,
                                                DISPATCH_DATA_DESTRUCTOR_FREE);

    NSError* error;
    shader.library = [m_device newLibraryWithData:data error:&error];
    if (error) {
        FATAL("GpuDeviceMetal: Failed to create shader: %s",
              error.localizedDescription.UTF8String);
    }

    shader.function = [shader.library newFunctionWithName:s_shaderEntryPointNames[type]];

    return shaderID;
}

void GpuDeviceMetal::DestroyShader(GpuShaderID shaderID)
{
    Shader& shader = m_shaderTable.Lookup(shaderID);
    [shader.function release];
    [shader.library release];
    m_shaderTable.Remove(shaderID);
}

GpuBufferID GpuDeviceMetal::CreateBuffer(GpuBufferType type,
                                         GpuBufferAccessMode accessMode,
                                         const void* data,
                                         unsigned size)
{
    GpuBufferID bufferID(m_bufferTable.Add());
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    MTLResourceOptions options = s_bufferAccessModeToResourceOptions[accessMode];
    if (data) {
        buffer.buffer = [m_device newBufferWithBytes:data
                                              length:size
                                             options:options];
    } else {
        buffer.buffer = [m_device newBufferWithLength:size options:options];
    }
    buffer.type = type;
    buffer.accessMode = accessMode;
    return bufferID;
}

void GpuDeviceMetal::DestroyBuffer(GpuBufferID bufferID)
{
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    [buffer.buffer release];
    m_bufferTable.Remove(bufferID);
}

void* GpuDeviceMetal::GetBufferContents(GpuBufferID bufferID)
{
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPUBUFFER_ACCESS_DYNAMIC);
    return [buffer.buffer contents];
}

void GpuDeviceMetal::FlushBufferRange(GpuBufferID bufferID, int start, int length)
{
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPUBUFFER_ACCESS_DYNAMIC);
    [buffer.buffer didModifyRange:NSMakeRange(start, length)];
}

GpuPipelineStateID GpuDeviceMetal::CreatePipelineStateObject(const GpuPipelineStateDesc& state)
{
    ASSERT(state.vertexShader != 0);
    ASSERT(state.pixelShader != 0);
    ASSERT(state.inputLayout != 0);

    GpuPipelineStateID pipelineStateID(m_pipelineStateTable.Add());
    PipelineStateObj& obj = m_pipelineStateTable.Lookup(pipelineStateID);

    obj.state = CreateMTLRenderPipelineState(state);
    obj.depthStencilState = CreateMTLDepthStencilState(state);

    return pipelineStateID;
}

id<MTLRenderPipelineState> GpuDeviceMetal::CreateMTLRenderPipelineState(const GpuPipelineStateDesc& state)
{
    MTLRenderPipelineDescriptor* desc = [[[MTLRenderPipelineDescriptor alloc] init] autorelease];
    desc.vertexFunction = m_shaderTable.Lookup(state.vertexShader).function;
    desc.fragmentFunction = m_shaderTable.Lookup(state.pixelShader).function;
    desc.vertexDescriptor = m_inputLayoutTable.Lookup(state.inputLayout).descriptor;

    MTLRenderPipelineColorAttachmentDescriptor* colorDesc;
    colorDesc = [[[MTLRenderPipelineColorAttachmentDescriptor alloc] init] autorelease];
    colorDesc.pixelFormat = s_metalColorPixelFormats[m_deviceFormat.pixelColorFormat];
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

GpuRenderPassID GpuDeviceMetal::CreateRenderPassObject(const GpuRenderPassDesc& pass)
{
    m_currentDrawable = [[GetCAMetalLayer() nextDrawable] retain];

    GpuRenderPassID renderPassID(m_renderPassTable.Add());
    RenderPassObj& obj = m_renderPassTable.Lookup(renderPassID);

    MTLRenderPassColorAttachmentDescriptor* colorDesc;
    colorDesc = [[[MTLRenderPassColorAttachmentDescriptor alloc] init] autorelease];
    colorDesc.clearColor = {pass.clearR, pass.clearG, pass.clearB, pass.clearA};
    if (pass.flags & GpuRenderPassDesc::FLAG_PERFORM_CLEAR)
        colorDesc.loadAction = MTLLoadActionClear;
    colorDesc.storeAction = MTLStoreActionStore;

    MTLRenderPassDepthAttachmentDescriptor* depthDesc = nil;
    // Only create a depth attachment for the render pass if this device
    // actually has a depth buffer.
    if (m_depthBuf != nil) {
        depthDesc = [[[MTLRenderPassDepthAttachmentDescriptor alloc] init] autorelease];
        depthDesc.clearDepth = pass.clearDepth;
        if (pass.flags & GpuRenderPassDesc::FLAG_PERFORM_CLEAR)
            depthDesc.loadAction = MTLLoadActionClear;
        depthDesc.storeAction = MTLStoreActionStore;
    }

    obj.descriptor = [[MTLRenderPassDescriptor alloc] init];
    obj.descriptor.depthAttachment = depthDesc;
    [obj.descriptor.colorAttachments setObject:colorDesc atIndexedSubscript:0];

    return renderPassID;
}

GpuInputLayoutID GpuDeviceMetal::CreateInputLayout(int nVertexAttribs,
                                                   const GpuVertexAttribute* attribs,
                                                   int nVertexBuffers,
                                                   const unsigned* strides)
{
    GpuInputLayoutID inputLayoutID(m_inputLayoutTable.Add());
    InputLayout& layout = m_inputLayoutTable.Lookup(inputLayoutID);

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

    return inputLayoutID;
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

    MTLRenderPassColorAttachmentDescriptor* colorDesc;
    colorDesc = [pass.descriptor.colorAttachments objectAtIndexedSubscript:0];
    colorDesc.texture = m_currentDrawable.texture;
    ASSERT(colorDesc.texture.width == m_deviceFormat.resolutionX);
    ASSERT(colorDesc.texture.height == m_deviceFormat.resolutionY);

    pass.descriptor.depthAttachment.texture = m_depthBuf;
    ASSERT(m_depthBuf.width == m_deviceFormat.resolutionX);
    ASSERT(m_depthBuf.height == m_deviceFormat.resolutionY);

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

    id<MTLRenderCommandEncoder> encoder = PreDraw(renderPass, viewport);

    for (int drawItemIndex = 0; drawItemIndex < nItems; ++drawItemIndex) {
        const GpuDrawItem* item = items[drawItemIndex];
        ASSERT(item != NULL);
        const GpuDrawItemHeader* header = (const GpuDrawItemHeader*)item;

        // Set the pipeline state
        ASSERT(m_pipelineStateTable.Has(header->pipelineStateID));
        PipelineStateObj& pipelineState
            = m_pipelineStateTable.Lookup(header->pipelineStateID);
        [encoder setRenderPipelineState:pipelineState.state];
        [encoder setDepthStencilState:pipelineState.depthStencilState];

        // Set vertex buffers
        const GpuDrawItemHeader::VertexBufEntry* vertexBuffers
            = header->GetVertexBufferArray();
        for (int i = 0; i < header->nVertexBuffers; ++i) {
            GpuBufferID bufferID(vertexBuffers[i].bufferID);
            ASSERT(m_bufferTable.Has(bufferID));
            unsigned offset = vertexBuffers[i].offset;
            [encoder setVertexBuffer:m_bufferTable.Lookup(bufferID).buffer
                              offset:offset
                             atIndex:(GPU_MAX_CBUFFERS + i)];
        }

        // Set cbuffers
        const u32* cbuffers = header->GetCBufferArray();
        for (int i = 0; i < header->nCBuffers; ++i) {
            GpuBufferID bufferID(cbuffers[i]);
            ASSERT(m_bufferTable.Has(bufferID));
            id<MTLBuffer> buf = m_bufferTable.Lookup(bufferID).buffer;
            [encoder setVertexBuffer:buf offset:0 atIndex:i];
            [encoder setFragmentBuffer:buf offset:0 atIndex:i];
        }

        // Submit the draw call
        MTLPrimitiveType primType = s_metalPrimitiveTypes[header->GetPrimitiveType()];
        if (header->IsIndexed()) {
            ASSERT(m_bufferTable.Has(header->indexBufferID));
            GpuIndexType gpuIndexType = header->GetIndexType();
            id<MTLBuffer> indexBuf = m_bufferTable.Lookup(header->indexBufferID).buffer;
            NSUInteger indexBufOffset = header->indexBufferOffset;
            indexBufOffset += header->first * s_indexTypeByteSizes[gpuIndexType];
            [encoder drawIndexedPrimitives:primType
                                indexCount:header->count
                                 indexType:s_metalIndexTypes[gpuIndexType]
                               indexBuffer:indexBuf
                         indexBufferOffset:indexBufOffset];
        } else {
            [encoder drawPrimitives:primType
                        vertexStart:header->first
                        vertexCount:header->count];
        }
    }

    PostDraw(encoder);
}

void GpuDeviceMetal::SceneBegin()
{
    @autoreleasepool {
        m_commandBuffer = [[m_commandQueue commandBuffer] retain];

        // Hack: -[CAMetalLayer nextDrawable] shouldn't be returning nil.
        // But sometimes it does -- maybe there is an actual reason why,
        // but for now, we just keep looping until it returns an actual
        // drawable.
        do {
            m_currentDrawable = [[GetCAMetalLayer() nextDrawable] retain];
        } while (!m_currentDrawable);
    }
}

void GpuDeviceMetal::ScenePresent()
{
    [m_commandBuffer presentDrawable:m_currentDrawable];
    [m_commandBuffer commit];

    [m_currentDrawable release];
    [m_commandBuffer release];

    ++m_frameNumber;
}

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

GpuShaderID GpuDevice::CreateShader(GpuShaderType type, const char* code, size_t length)
{ return Cast(this)->CreateShader(type, code, length); }

void GpuDevice::DestroyShader(GpuShaderID shaderID)
{ Cast(this)->DestroyShader(shaderID); }

GpuBufferID GpuDevice::CreateBuffer(GpuBufferType type,
                                    GpuBufferAccessMode accessMode,
                                    const void* data,
                                    unsigned size)
{ return Cast(this)->CreateBuffer(type, accessMode, data, size); }

void GpuDevice::DestroyBuffer(GpuBufferID bufferID)
{ Cast(this)->DestroyBuffer(bufferID); }

void* GpuDevice::GetBufferContents(GpuBufferID bufferID)
{ return Cast(this)->GetBufferContents(bufferID); }

void GpuDevice::FlushBufferRange(GpuBufferID bufferID, int start, int length)
{ Cast(this)->FlushBufferRange(bufferID, start, length); }

GpuPipelineStateID GpuDevice::CreatePipelineStateObject(const GpuPipelineStateDesc& state)
{ return Cast(this)->CreatePipelineStateObject(state); }

GpuRenderPassID GpuDevice::CreateRenderPassObject(const GpuRenderPassDesc& pass)
{ return Cast(this)->CreateRenderPassObject(pass); }

GpuInputLayoutID GpuDevice::CreateInputLayout(int nVertexAttribs,
                                              const GpuVertexAttribute* attribs,
                                              int nVertexBuffers,
                                              const unsigned* strides)
{ return Cast(this)->CreateInputLayout(nVertexAttribs, attribs, nVertexBuffers, strides); }

void GpuDevice::Draw(const GpuDrawItem* const* items,
                     int nItems,
                     GpuRenderPassID renderPass,
                     const GpuViewport& viewport)
{ Cast(this)->Draw(items, nItems, renderPass, viewport); }

void GpuDevice::SceneBegin()
{ Cast(this)->SceneBegin(); }

void GpuDevice::ScenePresent()
{ Cast(this)->ScenePresent(); }
