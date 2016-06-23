#include "GpuDevice/GpuDrawItemPool.h"

#include "Core/Macros.h"

GpuDrawItemPool::GpuDrawItemPool(GpuDevice& device, const GpuDrawItemWriterDesc& desc)
    : m_device(device)
    , m_desc(desc)
    , m_itemSize(8 + GpuDrawItemWriter::SizeInBytes(desc))
    , m_data()
    , m_freeIndex(0xFFFFFFFF)
{}

static void* Alloc(size_t size, void* userdata)
{
    return userdata;
}

GpuDrawItemPoolIndex GpuDrawItemPool::BeginDrawItem(GpuDrawItemWriter& writer,
                                                    GpuDrawItemPoolIndex prev)
{
    GpuDrawItemPoolIndex index;

    if (m_freeIndex != 0xFFFFFFFF) {
        index = GpuDrawItemPoolIndex(m_freeIndex);

        void* pos = &m_data[m_freeIndex * m_itemSize];
        memcpy(&m_freeIndex, pos, 4);

        writer.Begin(&m_device, m_desc, Alloc, (u8*)pos + 8);
    } else {
        ASSERT(m_data.size() % m_itemSize == 0);
        ASSERT(((m_data.size() / m_itemSize) + 1) < 0xFFFFFFFF &&
               "Pool doesn't have space for another draw item");

        size_t size = m_data.size();
        index = GpuDrawItemPoolIndex((u32)(size / m_itemSize));

        m_data.resize(size + m_itemSize);
        writer.Begin(&m_device, m_desc, Alloc, (u8*)&m_data[size] + 8);
    }

    // Set the previous item's next pointer to the new item
    if (prev != 0xFFFFFFFF) {
        u32 u32Index = index;
        memcpy(&m_data[prev * m_itemSize], &u32Index, 4);
    }

    // Set the new item's next pointer to 0xFFFFFFFF
    u32 u32Null = 0xFFFFFFFF;
    memcpy(&m_data[index * m_itemSize], &u32Null, 4);

    // Set the new item's prev pointer to the previous item
    u32 u32Prev = prev;
    memcpy(&m_data[index * m_itemSize] + 4, &u32Prev, 4);

    return index;
}

void GpuDrawItemPool::DeleteDrawItem(GpuDrawItemPoolIndex index)
{
    u8* pos = &m_data[index * m_itemSize];

    GpuDrawItem* drawItem = (GpuDrawItem*)(pos + 8);
    GPUDEVICE_UNREGISTER_DRAWITEM(m_device, drawItem);

    u32 next;
    memcpy(&next, pos, 4);

    u32 prev;
    memcpy(&prev, pos + 4, 4);

    // Set the previous item's next pointer to the item-to-be-deleted's next pointer
    if (prev != 0xFFFFFFFF)
        memcpy(&m_data[prev * m_itemSize], &next, 4);

    // Set the next item's prev pointer to the item-to-be-deleted's prev pointer
    if (next != 0xFFFFFFFF)
        memcpy(&m_data[next * m_itemSize] + 4, &prev, 4);

    // Set the item-to-be-deleted's next pointer to the free list pointer
    memcpy(&m_data[index * m_itemSize], &m_freeIndex, 4);

    // Update the free list pointer
    m_freeIndex = index;
}

GpuDrawItemPoolIndex GpuDrawItemPool::Next(GpuDrawItemPoolIndex index) const
{
    u32 next;
    memcpy(&next, &m_data[index * m_itemSize], sizeof next);
    return GpuDrawItemPoolIndex(next);
}

const GpuDrawItem* GpuDrawItemPool::GetDrawItem(GpuDrawItemPoolIndex index) const
{
    return (const GpuDrawItem*)(&m_data[index * m_itemSize] + 8);
}
