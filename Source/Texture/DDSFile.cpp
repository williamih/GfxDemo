#include "Texture/DDSFile.h"

#include <string.h>
#include <stdio.h>

#include "Core/Macros.h"
#include "Core/Endian.h"

struct DDSPixelFormat {
    u32 dwSize;
    u32 dwFlags;
    u32 dwFourCC;
    u32 dwRGBBitCount;
    u32 dwRBitMask;
    u32 dwGBitMask;
    u32 dwBBitMask;
    u32 dwABitMask;

    void FixEndian()
    {
        dwSize = EndianSwapLE32(dwSize);
        dwFlags = EndianSwapLE32(dwFlags);
        dwFourCC = EndianSwapLE32(dwFourCC);
        dwRGBBitCount = EndianSwapLE32(dwRGBBitCount);
        dwRBitMask = EndianSwapLE32(dwRBitMask);
        dwGBitMask = EndianSwapLE32(dwGBitMask);
        dwBBitMask = EndianSwapLE32(dwBBitMask);
        dwABitMask = EndianSwapLE32(dwABitMask);
    }
};

struct DDSHeader {
    u32 dwMagic;
    u32 dwSize;
    u32 dwFlags;
    u32 dwHeight;
    u32 dwWidth;
    u32 dwPitchOrLinearSize;
    u32 dwDepth;
    u32 dwMipMapCount;
    u32 dwReserved1[11];
    DDSPixelFormat ddspf;
    u32 dwCaps;
    u32 dwCaps2;
    u32 dwCaps3;
    u32 dwCaps4;
    u32 dwReserved2;

    void FixEndian()
    {
        dwMagic = EndianSwapLE32(dwMagic);
        dwSize = EndianSwapLE32(dwSize);
        dwFlags = EndianSwapLE32(dwFlags);
        dwHeight = EndianSwapLE32(dwHeight);
        dwWidth = EndianSwapLE32(dwWidth);
        dwPitchOrLinearSize = EndianSwapLE32(dwPitchOrLinearSize);
        dwDepth = EndianSwapLE32(dwDepth);
        dwMipMapCount = EndianSwapLE32(dwMipMapCount);
        ddspf.FixEndian();
        dwCaps = EndianSwapLE32(dwCaps);
        dwCaps2 = EndianSwapLE32(dwCaps2);
        dwCaps3 = EndianSwapLE32(dwCaps3);
        dwCaps4 = EndianSwapLE32(dwCaps4);
    }
};

const unsigned DDSD_MIPMAPCOUNT = 0x20000;
const unsigned DDPF_FOURCC = 0x4;

#define FOURCC(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

DDSFile::DDSFile(u8* data, int size, const char* path,
                 PFnDestroy destroy, void* userdata)
    : m_texFormat(DDSTEXFORMAT_UNKNOWN)
    , m_data(data)
    , m_destroyFunc(destroy)
    , m_destroyUserdata(userdata)
{
    DDSHeader* header = (DDSHeader*)&m_data[0];
    header->FixEndian();

    if (header->dwSize != 124)
        FATAL("Invalid DDS header (%s)", path);

    if (header->dwCaps2 != 0)
        FATAL("Non-2D DDS textures unsupported (%s)", path);

    m_texFormat = DDSTEXFORMAT_UNKNOWN;
    if (header->ddspf.dwFlags & DDPF_FOURCC) {
        switch (header->ddspf.dwFourCC) {
            case FOURCC('D', 'X', 'T', '1'):
                m_texFormat = DDSTEXFORMAT_DXT1;
                break;
            case FOURCC('D', 'X', 'T', '3'):
                m_texFormat = DDSTEXFORMAT_DXT3;
                break;
            case FOURCC('D', 'X', 'T', '5'):
                m_texFormat = DDSTEXFORMAT_DXT5;
                break;
            default:
                break;
        }
    }

    if (m_texFormat == DDSTEXFORMAT_UNKNOWN)
        FATAL("Unsupported DDS pixel format (%s)", path);
}

DDSFile::~DDSFile()
{
    m_destroyFunc(m_data, m_destroyUserdata);
}

unsigned DDSFile::Width() const
{
    DDSHeader* header = (DDSHeader*)&m_data[0];
    return header->dwWidth;
}

unsigned DDSFile::Height() const
{
    DDSHeader* header = (DDSHeader*)&m_data[0];
    return header->dwHeight;
}

unsigned DDSFile::MipCount() const
{
    DDSHeader* header = (DDSHeader*)&m_data[0];
    if (header->dwFlags & DDSD_MIPMAPCOUNT)
        return header->dwMipMapCount;
    return 1;
}

DDSTextureFormat DDSFile::Format() const
{
    return m_texFormat;
}

const u8* DDSFile::Pixels() const
{
    return (const u8*)(&m_data[0] + sizeof(DDSHeader));
}
