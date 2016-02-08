#ifndef ASSET_ASSETCACHE_H
#define ASSET_ASSETCACHE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <stdio.h>

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
    {}

    ~AssetCache()
    {
        PurgeUnreferencedAssets();
        if (m_map.size() != 0)
            printf("AssetCache: warning - %d asset(s) not destroyed\n", (int)m_map.size());
    }

    T* FindOrLoad(const char* path)
    {
        std::string strPath(path);

        typename AssetMap::iterator iter = m_map.find(strPath);
        if (iter != m_map.end())
            return iter->second;

        T* asset = m_factory.Create(path, m_loader);
        asset->AddRef();

        m_map[strPath] = asset;

        return asset;
    }

    void PurgeUnreferencedAssets()
    {
        // Todo: redesign the data structures used for this class so that
        // we don't have to iterate through a hashtable (SLOW!) in this function
        // and in UpdateRefreshSystem(). (The std::unordered_map implementation
        // is a hashtable).
        typename AssetMap::iterator iter = m_map.begin();
        const typename AssetMap::iterator end = m_map.end();
        while (iter != end) {
            if (iter->second->RefCount() == 1) {
                iter->second->Release();
                typename AssetMap::iterator next = iter;
                ++next;
                m_map.erase(iter);
                iter = next;
            } else {
                ++iter;
            }
        }
    }

#ifdef ASSET_REFRESH
    void Refresh(const char* path)
    {
        std::string strPath(path);

        auto iter = m_map.find(strPath);
        ASSERT(iter != m_map.end());

        T* asset = iter->second;
        m_factory.Refresh(asset, path, m_loader);
    }
#endif

#ifdef ASSET_REFRESH
    void UpdateRefreshSystem()
    {
        typename AssetMap::iterator iter = m_map.begin();
        const typename AssetMap::iterator end = m_map.end();
        for ( ; iter != end; ++iter) {
            iter->second->UpdateRefreshedStatus();
        }
    }
#endif
private:
    AssetCache(const AssetCache&);
    AssetCache& operator=(const AssetCache&);

    typedef std::unordered_map<std::string, T*> AssetMap;

    FileLoader& m_loader;
    AssetFactory<T>& m_factory;
    AssetMap m_map;
};

#endif // ASSET_ASSETCACHE_H
