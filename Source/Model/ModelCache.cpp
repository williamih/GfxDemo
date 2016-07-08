#include "Model/ModelCache.h"
#include "Core/Macros.h"
#include "Model/ModelInstance.h"

namespace {
    struct GetSharedKey {
        HashKey_Str operator()(ModelShared* shared) const
        {
            HashKey_Str key;
            key.str = shared->GetPath();
            return key;
        }
    };
}

ModelCache::ModelCache()
    : m_list()
    , m_hash()
{}

ModelCache::~ModelCache()
{
    RemoveUnused();
    ASSERT(m_list.Head() == NULL);
    ASSERT(m_hash.Count() == 0);
}

ModelShared* ModelCache::Get(
    GpuDevice& device,
    TextureCache& textureCache,
    FileLoader& loader,
    const char* path
)
{
    HashKey_Str key;
    key.str = path;

    if (ModelShared* const* ppShared = m_hash.Get(key, GetSharedKey()))
        return *ppShared;

    ModelShared* shared = ModelShared::Create(device, textureCache, loader, path);

    m_list.InsertTail(shared);
    m_hash.Insert(shared, GetSharedKey());
    
    return shared;
}

void ModelCache::Reload(
    GpuDevice& device,
    TextureCache& textureCache,
    FileLoader& loader,
    const char* path
)
{
    HashKey_Str key;
    key.str = path;

    ModelShared* const* ppShared = m_hash.Get(key, GetSharedKey());
    if (!ppShared)
        return;

    ModelShared* shared = *ppShared;

    ModelShared* successor = ModelShared::Create(device, textureCache, loader, path);

    ModelInstance* instance = shared->GetFirstInstance();
    for ( ; instance; instance = instance->NextInAssetGroup()) {
        instance->Reload(successor);
    }

    m_list.InsertTail(successor);
    m_hash.Insert(successor, GetSharedKey());

    ASSERT(shared->RefCount() == 0);
    ModelShared::Destroy(shared); // Automatically unlinks from the list
}

void ModelCache::RemoveUnused()
{
    for (ModelShared* shared = m_list.Head(); shared; ) {
        ModelShared* next = shared->m_link.Next();

        if (shared->RefCount() == 0) {
            HashKey_Str key;
            key.str = shared->GetPath();
            ASSERT(m_hash.Delete(key, GetSharedKey()));
            ModelShared::Destroy(shared);
        }

        shared = next;
    }
}
