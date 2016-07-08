#ifndef SHADER_SHADERASSET_H
#define SHADER_SHADERASSET_H

#include "Core/List.h"
#include "Core/Hash.h"
#include "Core/HashTypes.h"
#include "GpuDevice/GpuDevice.h"
#include "Asset/AssetRefreshQueue.h"

class FileLoader;

class ShaderAsset {
public:
    GpuShaderProgramID GetGpuShaderProgramID() const;

    int RefCount() const;
    void AddRef();
    void Release();

    const char* GetName() const;

    // Returns the old shader program ID
    GpuShaderProgramID Refresh(FileLoader& loader);
    bool PollRefreshed() const;

    // For use by the ShaderCache class -- these shouldn't need to be called
    // by user code.
    void ClearRefreshedStatus();
    static ShaderAsset* Create(GpuDevice& device, FileLoader& loader,
                               const char* name);
    static void Destroy(ShaderAsset* shader);

    LIST_LINK(ShaderAsset) m_link;

private:
    ShaderAsset(GpuDevice& device, FileLoader& loader, const char* name);
    ~ShaderAsset();
    ShaderAsset(const ShaderAsset&);
    ShaderAsset& operator=(const ShaderAsset&);

    GpuDevice& m_device;
    GpuShaderProgramID m_shaderProgram;
    int m_refCountAndRefreshStatus;
};

class ShaderCache {
public:
    ShaderCache(GpuDevice& device, FileLoader& loader);
    ~ShaderCache();

    ShaderAsset* FindOrLoad(const char* name);

    void RemoveUnusedShaders();

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

    LIST_DECLARE(ShaderAsset, m_link) m_shaderList;
    THash<HashKey_Str, ShaderAsset*> m_shaderHash;

    AssetRefreshQueue<ShaderAsset, GpuShaderProgramID> m_refreshQueue;
};

#endif // SHADER_SHADERASSET_H
