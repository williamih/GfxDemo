#include "GpuDevice/GpuSamplerCache.h"
#include "Core/Macros.h"

// Map from GpuSamplerCache::FilterQuality to GpuSamplerMipFilter
static const GpuSamplerMipFilter s_mipFilters[] = {
    GPU_SAMPLER_MIPFILTER_NEAREST,
    GPU_SAMPLER_MIPFILTER_LINEAR,
    GPU_SAMPLER_MIPFILTER_LINEAR,
};

// Indicates that we need to call the callbacks on the next opportunity.
const u32 FLAG_PENDING_CALLBACKS = 1;

GpuSamplerCache::GpuSamplerCache(GpuDevice& device)
    : m_device(device)
    , m_samplers()
    , m_callbacks()
    , m_quality(BILINEAR)
    , m_maxAnisotropy(1)
    , m_flags(0)
{}

GpuSamplerCache::~GpuSamplerCache()
{
    ASSERT(m_samplers.empty() && "Samplers not released");
}

GpuSamplerCache::FilterQuality GpuSamplerCache::GetFilterQuality() const
{
    return m_quality;
}

int GpuSamplerCache::GetMaxAnisotropy() const
{
    return m_maxAnisotropy;
}

void GpuSamplerCache::SetFilterQuality(FilterQuality quality, int maxAnisotropy)
{
    ASSERT(quality == ANISOTROPIC || maxAnisotropy == 1);
    m_quality = quality;
    m_maxAnisotropy = maxAnisotropy;
    m_flags |= FLAG_PENDING_CALLBACKS;
}

void GpuSamplerCache::RegisterCallback(PFnQualityChanged callback, void* userdata)
{
    CallbackInfo info = {callback, userdata};
    m_callbacks.push_back(info);
}

void GpuSamplerCache::UnregisterCallback(PFnQualityChanged callback, void* userdata)
{
    for (size_t i = 0; i < m_callbacks.size(); ++i) {
        if (m_callbacks[i].callback == callback
            && m_callbacks[i].userdata == userdata) {
            m_callbacks[i] = m_callbacks.back();
            m_callbacks.pop_back();
        }
    }
}

void GpuSamplerCache::CallCallbacks()
{
    if (m_flags & FLAG_PENDING_CALLBACKS) {
        for (size_t i = 0; i < m_callbacks.size(); ++i) {
            CallbackInfo& cb = m_callbacks[i];
            cb.callback(*this, cb.userdata);
        }
        m_flags &= ~(FLAG_PENDING_CALLBACKS);
    }
}

static GpuSamplerID CreateSampler(
    GpuDevice& device,
    GpuSamplerAddressMode address,
    GpuSamplerCache::FilterQuality quality,
    int maxAnisotropy
)
{
    GpuSamplerDesc desc;
    desc.uAddressMode = address;
    desc.vAddressMode = address;
    desc.wAddressMode = address;
    desc.minFilter = GPU_SAMPLER_FILTER_LINEAR;
    desc.magFilter = GPU_SAMPLER_FILTER_LINEAR;
    desc.mipFilter = s_mipFilters[quality];
    desc.maxAnisotropy = maxAnisotropy;
    return device.SamplerCreate(desc);
}

GpuSamplerID GpuSamplerCache::Acquire(GpuSamplerAddressMode address)
{
    for (size_t i = 0; i < m_samplers.size(); ++i) {
        if (m_samplers[i].addressMode == address &&
            m_samplers[i].quality == m_quality &&
            m_samplers[i].maxAnisotropy == m_maxAnisotropy) {
            ++m_samplers[i].refCount;
            return m_samplers[i].sampler;
        }
    }
    m_samplers.resize(m_samplers.size() + 1);
    SamplerInfo& s = m_samplers.back();
    s.refCount = 1;
    s.addressMode = address;
    s.quality = m_quality;
    s.maxAnisotropy = m_maxAnisotropy;
    s.sampler = CreateSampler(m_device, address, m_quality, m_maxAnisotropy);
    return s.sampler;
}

void GpuSamplerCache::Release(GpuSamplerID sampler)
{
    for (size_t i = 0; i < m_samplers.size(); ++i) {
        if (m_samplers[i].sampler == sampler) {
            if (!--m_samplers[i].refCount) {
                m_device.SamplerDestroy(m_samplers[i].sampler);
                m_samplers[i] = m_samplers.back();
                m_samplers.pop_back();
            }
            return;
        }
    }
    ASSERT(!"Sampler wasn't part of the cache");
}
