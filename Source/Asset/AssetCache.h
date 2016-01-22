#ifndef ASSET_ASSETCACHE_H
#define ASSET_ASSETCACHE_H

#include <string>
#include <memory>
#include <unordered_map>

#include "Core/Macros.h"
#include "File.h"

template<class T>
class AssetCache {
public:
    AssetCache()
        : m_map()
    {}

    template<class Factory>
    std::shared_ptr<T> FindOrLoad(const char* path, const Factory& f)
    {
        std::string strPath(path);

        auto iter = m_map.find(strPath);
        if (iter != m_map.end())
            return iter->second;

        std::vector<char> fileData;
        if (!FileReadFile(path, fileData))
            FATAL("Failed to load asset %s.", path);

        std::shared_ptr<T> asset(f((u8*)&fileData[0], (int)fileData.size()));

        m_map[strPath] = asset;

        return asset;
    }
private:
    AssetCache(const AssetCache&);
    AssetCache& operator=(const AssetCache&);

    std::unordered_map<std::string, std::shared_ptr<T>> m_map;
};

#endif // ASSET_ASSETCACHE_H
