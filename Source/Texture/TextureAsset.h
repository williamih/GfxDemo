#ifndef TEXTURE_TEXTUREASSET_H
#define TEXTURE_TEXTUREASSET_H

#include <unordered_map>
#include <string>
#include "GpuDevice/GpuDevice.h"
#include "Asset/AssetRefreshQueue.h"

class FileLoader;
class DDSFile;

class TextureAsset {
public:
    TextureAsset(GpuDevice& device, DDSFile& file);
    ~TextureAsset();

    GpuTextureID GetGpuTextureID() const;

    int RefCount() const;
    void AddRef();
    void Release();

    // Returns the old GpuTextureID
    GpuTextureID Refresh(DDSFile& file);
    bool PollRefreshed() const;

private:
    TextureAsset(const TextureAsset&);
    TextureAsset& operator=(const TextureAsset&);

    friend class TextureCache;
    void ClearRefreshedStatus();

    GpuDevice& m_device;
    GpuTextureID m_texture;
    int m_refCountAndRefreshStatus;
};

class TextureCache {
public:
    TextureCache(GpuDevice& device, FileLoader& loader);
    ~TextureCache();

    TextureAsset* FindOrLoad(const char* path);
    void Refresh(const char* path);
    void UpdateRefreshSystem();

private:
    TextureCache(const TextureCache&);
    TextureCache& operator=(const TextureCache&);

    static GpuTextureID RefreshPerform(
        TextureAsset* texture, const char* path, void* userdata
    );
    static void RefreshFinalize(
        TextureAsset* texture, GpuTextureID oldProgram, void* userdata
    );

    GpuDevice& m_device;
    FileLoader& m_fileLoader;
    std::unordered_map<std::string, TextureAsset*> m_textures;
    AssetRefreshQueue<TextureAsset, GpuTextureID> m_refreshQueue;
};

#endif // TEXTURE_TEXTUREASSET_H
