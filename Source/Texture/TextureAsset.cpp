#include "Texture/TextureAsset.h"

#include "Core/Macros.h"
#include "Core/FileLoader.h"

#include "Texture/DDSFile.h"

namespace {
    struct GetTextureKey {
        HashKey_Str operator()(TextureAsset* texture) const
        {
            HashKey_Str key;
            key.str = texture->GetPath();
            return key;
        }
    };
}

const int REFRESH_STATUS_MASK = 0x80000000;
const int REF_COUNT_MASK = ~(REFRESH_STATUS_MASK);

static GpuTextureID CreateTexture(GpuDevice& device, DDSFile& file)
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
    GpuTextureID texture = device.TextureCreate(
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
        device.TextureUpload(texture, region, i, stride, pixels);
        pixels += mipSize;
    }

    return texture;
}

TextureAsset::TextureAsset(GpuDevice& device, DDSFile& file)
    : m_link()

    , m_device(device)
    , m_texture(CreateTexture(device, file))
    , m_refCountAndRefreshStatus(0)
{}

TextureAsset::~TextureAsset()
{
    m_device.TextureDestroy(m_texture);
}

GpuTextureID TextureAsset::GetGpuTextureID() const
{
    return m_texture;
}

int TextureAsset::RefCount() const
{
    return (m_refCountAndRefreshStatus & REF_COUNT_MASK);
}

void TextureAsset::AddRef()
{
    ++m_refCountAndRefreshStatus;
}

void TextureAsset::Release()
{
    ASSERT((m_refCountAndRefreshStatus & REF_COUNT_MASK) > 0);
    --m_refCountAndRefreshStatus;
}

const char* TextureAsset::GetPath() const
{
    return (const char*)(this + 1);
}

GpuTextureID TextureAsset::Refresh(DDSFile& file)
{
    GpuTextureID oldTex = m_texture;
    m_texture = CreateTexture(m_device, file);
    return oldTex;
}

bool TextureAsset::PollRefreshed() const
{
    return (m_refCountAndRefreshStatus & REFRESH_STATUS_MASK);
}

void TextureAsset::ClearRefreshedStatus()
{
    m_refCountAndRefreshStatus &= ~(REFRESH_STATUS_MASK);
}

TextureAsset* TextureAsset::Create(GpuDevice& device, DDSFile& file,
                                   const char* path)
{
    size_t pathLen = StrLen(path) + 1; // includes the null terminator ( + 1 )
    void* memory = malloc(sizeof(TextureAsset) + pathLen);

    // Copy over the path
    memcpy((u8*)memory + sizeof(TextureAsset), path, pathLen);

    // Create the texture
    return new (memory) TextureAsset(device, file);
}

void TextureAsset::Destroy(TextureAsset* texture)
{
    texture->~TextureAsset();
    free(texture);
}

static void* Alloc(u32 size, void* userdata)
{
    return malloc(size);
}

static void Destroy(void* ptr, void* userdata)
{
    free(ptr);
}

TextureCache::TextureCache(GpuDevice& device, FileLoader& loader)
    : m_device(device)
    , m_fileLoader(loader)

    , m_textureList()
    , m_textureHash()

    , m_refreshQueue(&TextureCache::RefreshPerform,
                     &TextureCache::RefreshFinalize, (void*)this)
{}

TextureCache::~TextureCache()
{
    m_refreshQueue.Clear();

    RemoveUnusedTextures();
    ASSERT(m_textureList.Head() == NULL);
    ASSERT(m_textureHash.Count() == 0);
}

TextureAsset* TextureCache::FindOrLoad(const char* path)
{
    HashKey_Str key;
    key.str = path;

    if (TextureAsset* const* ppTexture = m_textureHash.Get(key, GetTextureKey()))
        return *ppTexture;

    u8* data;
    u32 size;
    m_fileLoader.Load(path, &data, &size, Alloc, NULL);

    DDSFile file(data, size, path, &Destroy, NULL);

    TextureAsset* texture = TextureAsset::Create(m_device, file, path);

    m_textureList.InsertTail(texture);
    m_textureHash.Insert(texture, GetTextureKey());

    return texture;
}

void TextureCache::RemoveUnusedTextures()
{
    for (TextureAsset* texture = m_textureList.Head(); texture; ) {
        TextureAsset* next = texture->m_link.Next();

        if (texture->RefCount() == 0) {
            HashKey_Str key;
            key.str = texture->GetPath();
            ASSERT(m_textureHash.Delete(key, GetTextureKey()));
            TextureAsset::Destroy(texture);
        }

        texture = next;
    }
}

void TextureCache::Refresh(const char* path)
{
    HashKey_Str key;
    key.str = path;

    TextureAsset* const* ppTexture = m_textureHash.Get(key, GetTextureKey());
    if (!ppTexture)
        return;

    TextureAsset* texture = *ppTexture;

    texture->AddRef();
    m_refreshQueue.QueueRefresh(texture, path);
}

void TextureCache::UpdateRefreshSystem()
{
    m_refreshQueue.Update();
}

GpuTextureID TextureCache::RefreshPerform(TextureAsset* texture,
                                          const char* path,
                                          void* userdata)
{
    TextureCache* self = (TextureCache*)userdata;

    u8* data;
    u32 size;
    self->m_fileLoader.Load(path, &data, &size, Alloc, NULL);

    DDSFile file(data, size, path, &Destroy, NULL);

    GpuTextureID old = texture->Refresh(file);

    return old;
}

void TextureCache::RefreshFinalize(TextureAsset* texture,
                                   GpuTextureID oldTexture,
                                   void* userdata)
{
    TextureCache* self = (TextureCache*)userdata;

    if (oldTexture != GpuTextureID())
        self->m_device.TextureDestroy(oldTexture);
    texture->ClearRefreshedStatus();
    texture->Release();
}
