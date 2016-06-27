#ifndef MODEL_MODELCACHE_H
#define MODEL_MODELCACHE_H

#include <string>
#include <unordered_map>
#include <vector>

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
    std::unordered_map<std::string, ModelShared*> m_map;
};

#endif // MODEL_MODELCACHE_H
