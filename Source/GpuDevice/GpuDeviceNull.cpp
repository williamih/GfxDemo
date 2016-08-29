#ifdef GPUDEVICE_API_NULL

#include "GpuDevice/GpuDevice.h"

#include <stdio.h>

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

// -----------------------------------------------------------------------------
// Lookup tables for textures
// -----------------------------------------------------------------------------

static const bool s_textureTypeIsArray[] = {
    false, // GPU_TEXTURE_1D
    true, // GPU_TEXTURE_1D_ARRAY
    false, // GPU_TEXTURE_2D
    true, // GPU_TEXTURE_2D_ARRAY
    false, // GPU_TEXTURE_CUBE
    false, // GPU_TEXTURE_3D
};

// -----------------------------------------------------------------------------
// GpuDeviceNull class declaration
// -----------------------------------------------------------------------------

class GpuDeviceNull {
public:
    GpuDeviceNull(const GpuDeviceFormat& format, void* osViewHandle);
    ~GpuDeviceNull();

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
    void PipelineStateDestroy(GpuPipelineStateID pipelineStateID);

    // Render passes
    bool RenderPassExists(GpuRenderPassID renderPassID) const;
    GpuRenderPassID RenderPassCreate(const GpuRenderPassDesc& pass);
    void RenderPassDestroy(GpuRenderPassID renderPassID);

    // Submit an array of draw items
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
    GpuDeviceNull(const GpuDeviceNull&);
    GpuDeviceNull& operator=(const GpuDeviceNull&);

    struct PermutationApiData {
        PermutationApiData()
            : m_inUse(false)
        {}

        bool IsInUse() const
        {
            return m_inUse;
        }

        void LoadVertexShader(GpuDevice& device, const char* code, int length)
        {
            m_inUse = true;
        }

        void LoadPixelShader(GpuDevice& device, const char* code, int length)
        {}

        void Release()
        {
            m_inUse = false;
        }

        bool m_inUse;
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
        GpuBufferType type;
        GpuBufferAccessMode accessMode;
        int maxUpdatesPerFrame;
        u32 size;
        u32 bufOffset;
        void* memory;
    };

    struct PipelineStateObj {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
        u32 dbg_shaderProgram;
        u32 dbg_inputLayout;
#endif
    };

    struct RenderPassObj {
        bool usesRenderTarget;
    };

    struct InputLayout {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
#endif
    };

    struct Texture {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
#endif
    };

    struct Sampler {
#ifdef GPUDEVICE_DEBUG_MODE
        int dbg_refCount;
#endif
    };

    GpuDeviceFormat m_deviceFormat;

    int m_frameNumber;

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
// GpuDeviceNull implementation
// -----------------------------------------------------------------------------

GpuDeviceNull::GpuDeviceNull(const GpuDeviceFormat& format, void* osViewHandle)
    : m_deviceFormat(format)
    , m_frameNumber(0)

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
{}

GpuDeviceNull::~GpuDeviceNull()
{
    if (m_dbg_shaderCount != 0) {
        fprintf(stderr,
                "GpuDeviceNull: warning - %d shader(s) not destroyed\n",
                m_dbg_shaderCount);
    }
    if (m_dbg_bufferCount != 0) {
        fprintf(stderr,
                "GpuDeviceNull: warning - %d buffer(s) not destroyed\n",
                m_dbg_bufferCount);
    }
    if (m_dbg_textureCount != 0) {
        fprintf(stderr,
                "GpuDeviceNull: warning - %d textures(s) not destroyed\n",
                m_dbg_textureCount);
    }
    if (m_dbg_samplerCount != 0) {
        fprintf(stderr,
                "GpuDeviceNull: warning - %d sampler(s) not destroyed\n",
                m_dbg_samplerCount);
    }
    if (m_dbg_psoCount != 0) {
        fprintf(stderr,
                "GpuDeviceNull: warning - %d pipeline state object(s) "
                "not destroyed\n", m_dbg_psoCount);
    }
    if (m_dbg_renderPassCount != 0) {
        fprintf(stderr,
                "GpuDeviceNull: warning - %d render pass object(s) "
                "not destroyed\n", m_dbg_renderPassCount);
    }
    if (m_dbg_inputLayoutCount != 0) {
        fprintf(stderr,
                "GpuDeviceNull: warning - %d input layout(s) "
                "not destroyed\n", m_dbg_inputLayoutCount);
    }
}

void GpuDeviceNull::SetFormat(const GpuDeviceFormat& format)
{
    m_deviceFormat = format;
}

const GpuDeviceFormat& GpuDeviceNull::GetFormat() const
{
    return m_deviceFormat;
}

void GpuDeviceNull::OnWindowResized()
{
}

bool GpuDeviceNull::ShaderProgramExists(GpuShaderProgramID shaderProgramID) const
{
    return m_shaderProgramTable.Has(shaderProgramID);
}

GpuShaderProgramID GpuDeviceNull::ShaderProgramCreate(const char* data, size_t length)
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

void GpuDeviceNull::ShaderProgramDestroy(GpuShaderProgramID shaderProgramID)
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

bool GpuDeviceNull::BufferExists(GpuBufferID bufferID) const
{
    return m_bufferTable.Has(bufferID);
}

GpuBufferID GpuDeviceNull::BufferCreate(GpuBufferType type,
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
    buffer.maxUpdatesPerFrame = maxUpdatesPerFrame;
    buffer.size = size;
    buffer.bufOffset = 0;
    buffer.memory = malloc(size);

    ++m_dbg_bufferCount;

    return bufferID;
}

void GpuDeviceNull::BufferStreamResize(GpuBufferID bufferID, unsigned newSize)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_STREAM);

    buffer.size = newSize;
    free(buffer.memory);
    buffer.memory = malloc(newSize);
}

void GpuDeviceNull::BufferDestroy(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (buffer.dbg_refCount != 0) {
        FATAL("Can't destroy buffer as it still has %d draw "
              "item(s) referencing it", buffer.dbg_refCount);
    }
#endif

    free(buffer.memory);

    m_bufferTable.Remove(bufferID);

    --m_dbg_bufferCount;
}

void* GpuDeviceNull::BufferMap(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_DYNAMIC ||
           buffer.accessMode == GPU_BUFFER_ACCESS_STREAM);
    return buffer.memory;
}

void GpuDeviceNull::BufferUnmap(GpuBufferID bufferID)
{
    ASSERT(BufferExists(bufferID));
    Buffer& buffer = m_bufferTable.Lookup(bufferID);
    ASSERT(buffer.accessMode == GPU_BUFFER_ACCESS_DYNAMIC ||
           buffer.accessMode == GPU_BUFFER_ACCESS_STREAM);
}

bool GpuDeviceNull::TextureExists(GpuTextureID textureID) const
{
    return m_textureTable.Has(textureID);
}

GpuTextureID GpuDeviceNull::TextureCreate(GpuTextureType type,
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

    ++m_dbg_textureCount;

    return textureID;
}

void GpuDeviceNull::TextureDestroy(GpuTextureID textureID)
{
    ASSERT(TextureExists(textureID));
    Texture& tex = m_textureTable.Lookup(textureID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (tex.dbg_refCount != 0) {
        FATAL("Can't destroy texture as it still has %d draw "
              "item(s) referencing it", tex.dbg_refCount);
    }
#endif

    m_textureTable.Remove(textureID);

    --m_dbg_textureCount;
}

void GpuDeviceNull::TextureUpload(GpuTextureID textureID,
                                   const GpuRegion& region,
                                   int mipmapLevel,
                                   int stride,
                                   const void* bytes)
{
    ASSERT(TextureExists(textureID));
}

bool GpuDeviceNull::SamplerExists(GpuSamplerID samplerID) const
{
    return m_samplerTable.Has(samplerID);
}

GpuSamplerID GpuDeviceNull::SamplerCreate(const GpuSamplerDesc& desc)
{
    ASSERT(1 <= desc.maxAnisotropy && desc.maxAnisotropy <= 16);

    GpuSamplerID samplerID(m_samplerTable.Add());
    Sampler& sampler = m_samplerTable.Lookup(samplerID);
#ifdef GPUDEVICE_DEBUG_MODE
    sampler.dbg_refCount = 0;
#endif

    ++m_dbg_samplerCount;

    return samplerID;
}

void GpuDeviceNull::SamplerDestroy(GpuSamplerID samplerID)
{
    ASSERT(SamplerExists(samplerID));
    Sampler& sampler = m_samplerTable.Lookup(samplerID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (sampler.dbg_refCount != 0) {
        FATAL("Can't destroy sampler as it still has %d draw "
              "item(s) referencing it", sampler.dbg_refCount);
    }
#endif

    m_samplerTable.Remove(samplerID);

    --m_dbg_samplerCount;
}

bool GpuDeviceNull::InputLayoutExists(GpuInputLayoutID inputLayoutID) const
{
    return m_inputLayoutTable.Has(inputLayoutID);
}

GpuInputLayoutID GpuDeviceNull::InputLayoutCreate(int nVertexAttribs,
                                                   const GpuVertexAttribute* attribs,
                                                   int nVertexBuffers,
                                                   const unsigned* strides)
{
    GpuInputLayoutID inputLayoutID(m_inputLayoutTable.Add());
    InputLayout& layout = m_inputLayoutTable.Lookup(inputLayoutID);
#ifdef GPUDEVICE_DEBUG_MODE
    layout.dbg_refCount = 0;
#endif

    ++m_dbg_inputLayoutCount;

    return inputLayoutID;
}

void GpuDeviceNull::InputLayoutDestroy(GpuInputLayoutID inputLayoutID)
{
    ASSERT(InputLayoutExists(inputLayoutID));
    InputLayout& layout = m_inputLayoutTable.Lookup(inputLayoutID);

#ifdef GPUDEVICE_DEBUG_MODE
    if (layout.dbg_refCount != 0) {
        FATAL("Can't destroy input layout as it still has %d pipeline state "
              "object(s) referencing it", layout.dbg_refCount);
    }
#endif

    m_inputLayoutTable.Remove(inputLayoutID);

    --m_dbg_inputLayoutCount;
}

bool GpuDeviceNull::PipelineStateExists(GpuPipelineStateID pipelineStateID) const
{
    return m_pipelineStateTable.Has(pipelineStateID);
}

GpuPipelineStateID GpuDeviceNull::PipelineStateCreate(const GpuPipelineStateDesc& state)
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

    ++m_dbg_psoCount;

    return pipelineStateID;
}

void GpuDeviceNull::PipelineStateDestroy(GpuPipelineStateID pipelineStateID)
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

    m_pipelineStateTable.Remove(pipelineStateID);

    --m_dbg_psoCount;
}

bool GpuDeviceNull::RenderPassExists(GpuRenderPassID renderPassID) const
{
    return m_renderPassTable.Has(renderPassID);
}

GpuRenderPassID GpuDeviceNull::RenderPassCreate(const GpuRenderPassDesc& pass)
{
    ASSERT(pass.numRenderTargets >= 0);

    GpuRenderPassID renderPassID(m_renderPassTable.Add());
    RenderPassObj& obj = m_renderPassTable.Lookup(renderPassID);
    obj.usesRenderTarget = false;

    if (pass.numRenderTargets > 0) {
        ASSERT(pass.renderTargets != NULL);
        obj.usesRenderTarget = true;
        for (int i = 0; i < pass.numRenderTargets; ++i) {
            ASSERT(pass.renderTargets[i] != 0);
        }
    }

    ++m_dbg_renderPassCount;

    return renderPassID;
}

void GpuDeviceNull::RenderPassDestroy(GpuRenderPassID renderPassID)
{
    ASSERT(RenderPassExists(renderPassID));
    m_renderPassTable.Remove(renderPassID);

    --m_dbg_renderPassCount;
}

void GpuDeviceNull::Draw(const GpuDrawItem* const* items,
                          int nItems,
                          GpuRenderPassID renderPass,
                          const GpuViewport& viewport)
{
    ASSERT(items != NULL);
    ASSERT(RenderPassExists(renderPass));

    for (int drawItemIndex = 0; drawItemIndex < nItems; ++drawItemIndex) {
        const GpuDrawItem* item = items[drawItemIndex];
        ASSERT(item != NULL);
    }
}

void GpuDeviceNull::SceneBegin()
{
}

void GpuDeviceNull::ScenePresent()
{
    ++m_frameNumber;
}

#ifdef GPUDEVICE_DEBUG_MODE
void GpuDeviceNull::DrawItem_UpdateRefCounts(const GpuDrawItem* item, int increment)
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

void GpuDeviceNull::RegisterDrawItem(const GpuDrawItem* item)
{
    DrawItem_UpdateRefCounts(item, 1);
}

void GpuDeviceNull::UnregisterDrawItem(const GpuDrawItem* item)
{
    DrawItem_UpdateRefCounts(item, -1);
}
#endif // GPUDEVICE_DEBUG_MODE

static const GpuDeviceNull* Cast(const GpuDevice* dev) { return (const GpuDeviceNull*)dev; }
static GpuDeviceNull* Cast(GpuDevice* dev) { return (GpuDeviceNull*)dev; }
static GpuDevice* Cast(GpuDeviceNull* dev) { return (GpuDevice*)dev; }

GpuDevice* GpuDevice::Create(const GpuDeviceFormat& format, void* osViewHandle)
{ return Cast(new GpuDeviceNull(format, osViewHandle)); }

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
    return Matrix44();
}

Matrix44 GpuDevice::TransformCreatePerspective(float left, float right,
                                               float bot, float top,
                                               float near, float far)
{
    return Matrix44();
}

#endif // GPU_API_NULL
