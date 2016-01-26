#ifndef ASSET_ASSET_H
#define ASSET_ASSET_H

#include "Core/Types.h"

class Asset {
public:
#ifdef ASSET_REFRESH
    bool WasJustRefreshed() const;

    // Call at the end of every frame
    void UpdateRefreshedStatus();
#endif

protected:
    Asset();
    virtual ~Asset() {}

#ifdef ASSET_REFRESH
    void MarkRefreshed();
#endif

private:
    Asset(const Asset&);
    Asset& operator=(const Asset&);

#ifdef ASSET_REFRESH
    bool m_wasJustRefreshed;
#endif
};

template<class T>
class AssetFactory {
public:
    virtual ~AssetFactory() {}

    virtual T* Create(u8* data, int size) = 0;
#ifdef ASSET_REFRESH
    virtual void Refresh(T* asset, u8* data, int size) = 0;
#endif
};

#endif // ASSET_ASSET_H
