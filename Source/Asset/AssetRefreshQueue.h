#ifndef ASSET_ASSETREFRESHQUEUE_H
#define ASSET_ASSETREFRESHQUEUE_H

#include <vector>
#include <string.h>
#include <stdlib.h>

template<class T, class TValue>
class AssetRefreshQueue {
public:
    typedef TValue (*PFnRefresh)(T* asset, const char* path, void* userdata);
    typedef void (*PFnFinalize)(T* asset, TValue oldValue, void* userdata);

    AssetRefreshQueue(PFnRefresh refresh, PFnFinalize finalize, void* userdata)
        : m_refresh(refresh)
        , m_finalize(finalize)
        , m_userdata(userdata)
        , m_refreshList()
    {}

    ~AssetRefreshQueue()
    {
        Clear();
    }

    void Clear()
    {
        for (size_t i = 0; i < m_refreshList.size(); ++i) {
            if (m_refreshList[i].path != NULL)
                free(m_refreshList[i].path);
            m_finalize(m_refreshList[i].asset, m_refreshList[i].old, m_userdata);
        }
        m_refreshList.clear();
    }

    void QueueRefresh(T* asset, const char* path)
    {
        RefreshInfo info = {asset, TValue(), strdup(path)};
        m_refreshList.push_back(info);
    }

    void Update()
    {
        for (size_t i = 0; i < m_refreshList.size(); ) {
            RefreshInfo& info = m_refreshList[i];
            if (info.path != NULL) {
                info.old = m_refresh(info.asset, info.path, m_userdata);
                free(info.path);
                info.path = NULL;
                ++i;
            } else {
                m_finalize(info.asset, info.old, m_userdata);
                m_refreshList[i] = m_refreshList.back();
                m_refreshList.pop_back();
            }
        }
    }

private:
    AssetRefreshQueue(const AssetRefreshQueue&);
    AssetRefreshQueue& operator=(const AssetRefreshQueue&);

    struct RefreshInfo {
        T* asset;
        TValue old;
        char* path;
    };

    PFnRefresh m_refresh;
    PFnFinalize m_finalize;
    void* m_userdata;
    std::vector<RefreshInfo> m_refreshList;
};

#endif // ASSET_ASSETREFRESHQUEUE_H
