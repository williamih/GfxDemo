#ifndef GPUDEVICE_GPUDRAWITEMPOOL_H
#define GPUDEVICE_GPUDRAWITEMPOOL_H

#include <vector>
#include "Core/Types.h"
#include "GpuDevice/GpuDrawItemWriter.h"

DECLARE_PRIMITIVE_WRAPPER(u32, GpuDrawItemPoolIndex);

class GpuDrawItemPool {
public:
    GpuDrawItemPool(GpuDevice* device, const GpuDrawItemWriterDesc& desc);

    GpuDrawItemPoolIndex BeginDrawItem(GpuDrawItemWriter& writer,
                                       GpuDrawItemPoolIndex prev
                                           = GpuDrawItemPoolIndex(0xFFFFFFFF));
    void DeleteDrawItem(GpuDrawItemPoolIndex index);

    GpuDrawItemPoolIndex Next(GpuDrawItemPoolIndex index) const;

    // Important: if BeginDrawItem() is called after this function, the returned
    // pointer is invalidated (the internal storage may have been reallocated).
    const GpuDrawItem* GetDrawItem(GpuDrawItemPoolIndex index) const;
private:
    GpuDrawItemPool(const GpuDrawItemPool&);
    GpuDrawItemPool& operator=(const GpuDrawItemPool&);

    GpuDevice* m_device;
    GpuDrawItemWriterDesc m_desc;
    size_t m_itemSize;
    std::vector<u8> m_data;
    u32 m_freeIndex;
};

#endif // GPUDEVICE_GPUDRAWITEMPOOL_H
