#ifndef TEXTURE_TEXTUREASSET_H
#define TEXTURE_TEXTUREASSET_H

#include "Core/List.h"
#include "Core/Hash.h"
#include "Core/HashTypes.h"
#include "GpuDevice/GpuDevice.h"
#include "Asset/AssetRefreshQueue.h"

class FileLoader;
class DDSFile;

class TextureAsset {
public:
    GpuTextureID GetGpuTextureID() const;

    int RefCount() const;
    void AddRef();
    void Release();

    const char* GetPath() const;

    // Returns the old GpuTextureID
    GpuTextureID Refresh(DDSFile& file);
    bool PollRefreshed() const;

    // For use by the TextureCache class -- these shouldn't need to be called
    // by user code.
    void ClearRefreshedStatus();
    static TextureAsset* Create(GpuDevice& device, DDSFile& file, const char* path);
    static void Destroy(TextureAsset* texture);

    LIST_LINK(TextureAsset) m_link;

private:
    TextureAsset(GpuDevice& device, DDSFile& file);
    ~TextureAsset();
    TextureAsset(const TextureAsset&);
    TextureAsset& operator=(const TextureAsset&);

    GpuDevice& m_device;
    GpuTextureID m_texture;
    int m_refCountAndRefreshStatus;
};

class TextureCache {
public:
    TextureCache(GpuDevice& device, FileLoader& loader);
    ~TextureCache();

    TextureAsset* FindOrLoad(const char* path);

    void RemoveUnusedTextures();

    void Refresh(const char* path);
    void UpdateRefreshSystem();

private:
    TextureCache(const TextureCache&);
    TextureCache& operator=(const TextureCache&);

    static GpuTextureID RefreshPerform(
        TextureAsset* texture, void* userdata
    );
    static void RefreshFinalize(
        TextureAsset* texture, GpuTextureID oldProgram, void* userdata
    );

    GpuDevice& m_device;
    FileLoader& m_fileLoader;

    LIST_DECLARE(TextureAsset, m_link) m_textureList;
    THash<HashKey_Str, TextureAsset*> m_textureHash;

    AssetRefreshQueue<TextureAsset, GpuTextureID> m_refreshQueue;
};

#endif // TEXTURE_TEXTUREASSET_H
