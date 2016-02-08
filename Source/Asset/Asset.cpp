#include "Asset/Asset.h"
#include "Core/Macros.h"

Asset::Asset()
    : m_refCount(0)
#ifdef ASSET_REFRESH
    , m_wasJustRefreshed(false)
#endif
{}

void Asset::AddRef()
{
    ++m_refCount;
}

void Asset::Release()
{
    if (!--m_refCount)
        Destroy();
}

int Asset::RefCount() const
{
    return m_refCount;
}

void Asset::Destroy()
{
    delete this;
}

#ifdef ASSET_REFRESH
bool Asset::WasJustRefreshed() const
{
    return m_wasJustRefreshed;
}

void Asset::UpdateRefreshedStatus()
{
    m_wasJustRefreshed = false;
}

void Asset::MarkRefreshed()
{
    ASSERT(!m_wasJustRefreshed);
    m_wasJustRefreshed = true;
}
#endif
