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

    virtual void* Allocate(u32 size) = 0;

    // The AssetFactory takes ownership of the data pointer, and is responsible
    // for handling its (either immediate or eventual) deletion.
    // The data is always allocated via the factory's Allocate() function.
    virtual T* Create(u8* data, u32 size, const char* path) = 0;
#ifdef ASSET_REFRESH
    virtual void Refresh(T* asset, u8* data, u32 size, const char* path) = 0;
#endif
};

#endif // ASSET_ASSET_H
