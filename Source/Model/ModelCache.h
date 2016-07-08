#ifndef MODEL_MODELCACHE_H
#define MODEL_MODELCACHE_H

#include "Core/Hash.h"
#include "Core/HashTypes.h"
#include "Model/ModelShared.h"

class GpuDevice;
class FileLoader;
class TextureCache;
class ModelShared;

class ModelCache {
public:
    ModelCache();
    ~ModelCache();

    ModelShared* Get(
        GpuDevice& device,
        TextureCache& textureCache,
        FileLoader& loader,
        const char* path
    );
    void Reload(
        GpuDevice& device,
        TextureCache& textureCache,
        FileLoader& loader,
        const char* path
    );

    void RemoveUnused();

private:
    LIST_DECLARE(ModelShared, m_link) m_list;
    THash<HashKey_Str, ModelShared*> m_hash;
};

#endif // MODEL_MODELCACHE_H
