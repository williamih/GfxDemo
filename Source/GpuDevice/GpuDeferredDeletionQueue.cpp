#include "GpuDevice/GpuDeferredDeletionQueue.h"

#include "Core/Macros.h"

const u16 TYPE_SHADERPROGRAM = 0;
const u16 TYPE_TEXTURE = 1;

GpuDeferredDeletionQueue::GpuDeferredDeletionQueue()
    : m_items()
{}

void GpuDeferredDeletionQueue::AddShaderProgram(GpuShaderProgramID programID,
                                                u16 framesUntilDelete)
{
    Item item = {programID, framesUntilDelete, TYPE_SHADERPROGRAM};
    m_items.push_back(item);
}

void GpuDeferredDeletionQueue::AddTexture(GpuTextureID textureID,
                                          u16 framesUntilDelete)
{
    Item item = {textureID, framesUntilDelete, TYPE_TEXTURE};
    m_items.push_back(item);
}

static void Delete(u32 resourceID, u16 type, GpuDevice& device)
{
    switch (type) {
        case TYPE_SHADERPROGRAM: {
            GpuShaderProgramID programID(resourceID);
            device.ShaderProgramDestroy(programID);
            break;
        }
        case TYPE_TEXTURE: {
            GpuTextureID textureID(resourceID);
            device.TextureDestroy(textureID);
            break;
        }
        default:
            ASSERT(!"Unknown resource type");
            break;
    }
}

void GpuDeferredDeletionQueue::Update(GpuDevice& device)
{
    for (size_t i = 0; i < m_items.size(); ) {
        --m_items[i].framesUntilDelete;
        if (m_items[i].framesUntilDelete == 0) {
            Delete(m_items[i].resourceID, m_items[i].type, device);
            m_items[i] = m_items.back();
            m_items.pop_back();
        } else {
            ++i;
        }
    }
}
