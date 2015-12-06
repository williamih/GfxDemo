#ifndef MODEL_MODELCACHE_H
#define MODEL_MODELCACHE_H

#include <memory>
#include <string>
#include <unordered_map>

class GpuDevice;
class ModelAsset;

class ModelCache {
public:
    explicit ModelCache(GpuDevice* device);

    std::shared_ptr<ModelAsset> FindOrLoad(const char* path);
private:
    ModelCache(const ModelCache&);
    ModelCache& operator=(const ModelCache&);

    GpuDevice* m_device;
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> m_map;
};

#endif // MODEL_MODELCACHE_H
