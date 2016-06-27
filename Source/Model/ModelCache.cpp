#include "Model/ModelCache.h"
#include "Core/Macros.h"
#include "Model/ModelShared.h"
#include "Model/ModelInstance.h"

ModelCache::ModelCache()
    : m_map()
{}

ModelCache::~ModelCache()
{
    RemoveUnused();
    ASSERT(m_map.empty());
}

ModelShared* ModelCache::Get(
    GpuDevice& device,
    TextureCache& textureCache,
    FileLoader& loader,
    const char* path
)
{
    std::string strPath(path);

    auto iter = m_map.find(strPath);
    if (iter != m_map.end())
        return iter->second;

    ModelShared* shared = ModelShared::Create(device, textureCache, loader, path);

    m_map[path] = shared;
    
    return shared;
}

void ModelCache::Reload(
    GpuDevice& device,
    TextureCache& textureCache,
    FileLoader& loader,
    const char* path
)
{
    std::string strPath(path);

    auto iter = m_map.find(strPath);
    if (iter == m_map.end())
        return;

    ModelShared* shared = iter->second;

    ModelShared* successor = ModelShared::Create(device, textureCache, loader, path);

    ModelInstance* instance = shared->GetFirstInstance();
    for ( ; instance; instance = instance->NextInAssetGroup()) {
        instance->Reload(successor);
    }

    m_map[strPath] = successor;

    ASSERT(shared->RefCount() == 0);
    ModelShared::Destroy(shared);
}

void ModelCache::RemoveUnused()
{
    auto iter = m_map.begin();
    auto end = m_map.end();
    while (iter != end) {
        auto next = iter;
        ++next;
        if (iter->second->RefCount() == 0) {
            ModelShared::Destroy(iter->second);
            m_map.erase(iter);
        }
        iter = next;
    }
}
