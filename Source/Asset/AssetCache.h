#ifndef ASSET_ASSETCACHE_H
#define ASSET_ASSETCACHE_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/FileLoader.h"
#include "Asset/Asset.h"

template<class T>
class AssetCache {
public:
    AssetCache(FileLoader& loader, AssetFactory<T>& factory)
        : m_loader(loader)
        , m_factory(factory)
        , m_map()
#ifdef ASSET_REFRESH
        , m_assets()
#endif
    {}

    std::shared_ptr<T> FindOrLoad(const char* path)
    {
        std::string strPath(path);

        auto iter = m_map.find(strPath);
        if (iter != m_map.end())
            return iter->second;

        u8* data;
        u32 size;
        m_loader.Load(path, &data, &size, Alloc, &m_factory);

        std::shared_ptr<T> asset(m_factory.Create(data, size, path));

        m_map[strPath] = asset;
#ifdef ASSET_REFRESH
        m_assets.push_back(asset.get());
#endif

        return asset;
    }

#ifdef ASSET_REFRESH
    void Refresh(const char* path)
    {
        std::string strPath(path);

        auto iter = m_map.find(strPath);
        ASSERT(iter != m_map.end());

        u8* data;
        u32 size;
        m_loader.Load(path, &data, &size, Alloc, &m_factory);

        std::shared_ptr<T> asset = iter->second;
        m_factory.Refresh(asset.get(), data, size, path);
    }
#endif

#ifdef ASSET_REFRESH
    void UpdateRefreshSystem()
    {
        for (size_t i = 0; i < m_assets.size(); ++i) {
            m_assets[i]->UpdateRefreshedStatus();
        }
    }
#endif
private:
    AssetCache(const AssetCache&);
    AssetCache& operator=(const AssetCache&);

    static void* Alloc(u32 size, void* userdata)
    {
        return ((AssetFactory<T>*)userdata)->Allocate(size);
    }

    FileLoader& m_loader;
    AssetFactory<T>& m_factory;
    std::unordered_map<std::string, std::shared_ptr<T>> m_map;
#ifdef ASSET_REFRESH
    std::vector<T*> m_assets;
#endif
};

#endif // ASSET_ASSETCACHE_H
