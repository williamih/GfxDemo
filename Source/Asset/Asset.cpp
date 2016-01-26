#include "Asset/Asset.h"
#include "Core/Macros.h"

Asset::Asset()
#ifdef ASSET_REFRESH
    : m_wasJustRefreshed(false)
#endif
{}

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
