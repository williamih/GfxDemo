#ifndef SHADER_SHADERASSET_H
#define SHADER_SHADERASSET_H

#include <string>
#include <unordered_map>
#include "GpuDevice/GpuDevice.h"
#include "Asset/AssetRefreshQueue.h"

class FileLoader;

class ShaderAsset {
public:
    ShaderAsset(GpuDevice& device, const char* data, size_t length);
    ~ShaderAsset();

    GpuShaderProgramID GetGpuShaderProgramID() const;

    int RefCount() const;
    void AddRef();
    void Release();

    // Returns the old shader program ID
    GpuShaderProgramID Refresh(const char* data, size_t length);
    bool PollRefreshed() const;

private:
    ShaderAsset(const ShaderAsset&);
    ShaderAsset& operator=(const ShaderAsset&);

    friend class ShaderCache;
    void ClearRefreshedStatus();

    GpuDevice& m_device;
    GpuShaderProgramID m_shaderProgram;
    int m_refCountAndRefreshStatus;
};

class ShaderCache {
public:
    ShaderCache(GpuDevice& device, FileLoader& loader);
    ~ShaderCache();

    ShaderAsset* FindOrLoad(const char* name);
    void Refresh(const char* name);
    void UpdateRefreshSystem();

private:
    ShaderCache(const ShaderCache&);
    ShaderCache& operator=(const ShaderCache&);

    static GpuShaderProgramID RefreshPerform(
        ShaderAsset* shader, const char* path, void* userdata
    );
    static void RefreshFinalize(
        ShaderAsset* shader, GpuShaderProgramID oldProgram, void* userdata
    );

    GpuDevice& m_device;
    FileLoader& m_fileLoader;
    std::unordered_map<std::string, ShaderAsset*> m_shaders;
    AssetRefreshQueue<ShaderAsset, GpuShaderProgramID> m_refreshQueue;
};

#endif // SHADER_SHADERASSET_H
