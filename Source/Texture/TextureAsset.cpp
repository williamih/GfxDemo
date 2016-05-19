#include "Texture/TextureAsset.h"

#include "Core/Macros.h"
#include "Core/FileLoader.h"

#include "GpuDevice/GpuDeferredDeletionQueue.h"

#include "Texture/DDSFile.h"

static GpuTextureID CreateTexture(GpuDevice* device, DDSFile& file)
{
    GpuPixelFormat pixelFormat;
    unsigned bytesPerBlock;
    switch (file.Format()) {
        case DDSTEXFORMAT_DXT1:
            pixelFormat = GPU_PIXEL_FORMAT_DXT1;
            bytesPerBlock = 8;
            break;
        case DDSTEXFORMAT_DXT3:
            pixelFormat = GPU_PIXEL_FORMAT_DXT3;
            bytesPerBlock = 16;
            break;
        case DDSTEXFORMAT_DXT5:
            pixelFormat = GPU_PIXEL_FORMAT_DXT5;
            bytesPerBlock = 16;
            break;
        default: ASSERT(!"Unknown DDSTextureFormat"); break;
    }

    unsigned width = file.Width();
    unsigned height = file.Height();
    unsigned nMips = file.MipCount();
    GpuTextureID texture = device->TextureCreate(
        GPU_TEXTURE_2D,
        pixelFormat,
        0,
        width,
        height,
        1,
        file.MipCount()
    );

    const u8* pixels = file.Pixels();
    for (unsigned i = 0; i < nMips; ++i) {
        unsigned w = width >> i;
        unsigned h = height >> i;
        unsigned mipSize = w / 4 * h / 4 * bytesPerBlock;
        unsigned stride = w / 4 * bytesPerBlock;
        GpuRegion region;
        region.x = 0;
        region.y = 0;
        region.width = w;
        region.height = h;
        device->TextureUpload(texture, region, i, stride, pixels);
        pixels += mipSize;
    }

    return texture;
}

TextureAsset::TextureAsset(GpuDevice* device, DDSFile& file)
    : m_device(device)
    , m_texture(CreateTexture(device, file))
{}

TextureAsset::~TextureAsset()
{
    m_device->TextureDestroy(m_texture);
}

#ifdef ASSET_REFRESH
void TextureAsset::Refresh(GpuDeferredDeletionQueue& deletionQ, DDSFile& file)
{
    deletionQ.AddTexture(m_texture);
    m_texture = CreateTexture(m_device, file);
}
#endif

GpuTextureID TextureAsset::GetGpuTextureID() const
{
    return m_texture;
}

TextureAssetFactory::TextureAssetFactory(GpuDevice* device
#ifdef ASSET_REFRESH
                                         , GpuDeferredDeletionQueue& deletionQ
#endif
)
    : m_device(device)
#ifdef ASSET_REFRESH
    , m_deletionQ(deletionQ)
#endif
{}

static void* Alloc(u32 size, void* userdata)
{
    return malloc(size);
}

static void Destroy(void* ptr, void* userdata)
{
    free(ptr);
}

TextureAsset* TextureAssetFactory::Create(const char* path, FileLoader& loader)
{
    u8* data;
    u32 size;
    loader.Load(path, &data, &size, Alloc, NULL);
    DDSFile file(data, size, path, &Destroy, NULL);
    TextureAsset* asset = new TextureAsset(m_device, file);
    return asset;
}

#ifdef ASSET_REFRESH
void TextureAssetFactory::Refresh(TextureAsset* asset, const char* path, FileLoader& loader)
{
    u8* data;
    u32 size;
    loader.Load(path, &data, &size, Alloc, NULL);
    DDSFile file(data, size, path, &Destroy, NULL);
    asset->Refresh(m_deletionQ, file);
}
#endif
