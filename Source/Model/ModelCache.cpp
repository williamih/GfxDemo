#include "Model/ModelCache.h"
#include "Core/Macros.h"
#include "File.h"
#include "Model/ModelAsset.h"

ModelCache::ModelCache(GpuDevice* device)
    : m_device(device)
    , m_map()
{
    ASSERT(device);
}

std::shared_ptr<ModelAsset> ModelCache::FindOrLoad(const char* path)
{
    std::string strPath(path);

    auto iter = m_map.find(strPath);
    if (iter != m_map.end())
        return iter->second;

    std::vector<char> fileData;
    if (!FileReadFile(path, fileData))
        FATAL("Failed to load model %s.", path);

    std::shared_ptr<ModelAsset> model(new ModelAsset(m_device,
                                                     (u8*)&fileData[0],
                                                     (int)fileData.size()));
    m_map[strPath] = model;

    return model;
}
