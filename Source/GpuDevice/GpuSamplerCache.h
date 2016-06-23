#ifndef GPUDEVICE_GPUSAMPLERCACHE_H
#define GPUDEVICE_GPUSAMPLERCACHE_H

#include <vector>
#include "GpuDevice/GpuDevice.h"

class GpuSamplerCache {
public:
    enum FilterQuality {
        BILINEAR,
        TRILINEAR,
        ANISOTROPIC,
    };

    typedef void (*PFnQualityChanged)(GpuSamplerCache& cache, void* userdata);

    explicit GpuSamplerCache(GpuDevice& device);
    ~GpuSamplerCache();

    FilterQuality GetFilterQuality() const;
    int GetMaxAnisotropy() const;
    void SetFilterQuality(FilterQuality quality, int maxAnisotropy);

    void RegisterCallback(PFnQualityChanged callback, void* userdata);
    void UnregisterCallback(PFnQualityChanged callback, void* userdata);
    void CallCallbacks();

    GpuSamplerID Acquire(GpuSamplerAddressMode address);
    void Release(GpuSamplerID sampler);
private:
    struct SamplerInfo {
        int refCount;

        GpuSamplerAddressMode addressMode;
        FilterQuality quality;
        int maxAnisotropy;

        GpuSamplerID sampler;
    };

    struct CallbackInfo {
        PFnQualityChanged callback;
        void* userdata;
    };

    GpuDevice& m_device;
    std::vector<SamplerInfo> m_samplers;
    std::vector<CallbackInfo> m_callbacks;
    FilterQuality m_quality;
    int m_maxAnisotropy;
    u32 m_flags;
};

#endif // GPUDEVICE_GPUSAMPLERCACHE_H
