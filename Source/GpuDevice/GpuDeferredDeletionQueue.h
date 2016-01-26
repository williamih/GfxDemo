#ifndef GPUDEVICE_GPUDEFERREDDELETIONQUEUE_h
#define GPUDEVICE_GPUDEFERREDDELETIONQUEUE_h

#include <vector>
#include "GpuDevice/GpuDevice.h"

class GpuDeferredDeletionQueue {
public:
    GpuDeferredDeletionQueue();

    void AddShaderProgram(GpuShaderProgramID programID, u16 framesUntilDelete = 1);
    void AddTexture(GpuTextureID textureID, u16 framesUntilDelete = 1);

    // Call at the end of each frame
    void Update(GpuDevice* device);

private:
    GpuDeferredDeletionQueue(const GpuDeferredDeletionQueue&);
    GpuDeferredDeletionQueue& operator=(const GpuDeferredDeletionQueue&);

    struct Item {
        u32 resourceID;
        u16 framesUntilDelete;
        u16 type;
    };

    std::vector<Item> m_items;
};

#endif // GPUDEVICE_GPUDEFERREDDELETIONQUEUE_h
