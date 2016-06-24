#include "Shader/ShaderAsset.h"

#include <stdlib.h>

#include "Core/Macros.h"
#include "Core/FileLoader.h"

const int REFRESH_STATUS_MASK = 0x80000000;
const int REF_COUNT_MASK = ~(REFRESH_STATUS_MASK);

ShaderAsset::ShaderAsset(GpuDevice& device, const char* data, size_t length)
    : m_device(device)
    , m_shaderProgram(0)
    , m_refCountAndRefreshStatus(0)
{
    m_shaderProgram = device.ShaderProgramCreate(data, length);
}

ShaderAsset::~ShaderAsset()
{
    m_device.ShaderProgramDestroy(m_shaderProgram);
}

GpuShaderProgramID ShaderAsset::GetGpuShaderProgramID() const
{
    return m_shaderProgram;
}

int ShaderAsset::RefCount() const
{
    return (m_refCountAndRefreshStatus & REF_COUNT_MASK);
}

void ShaderAsset::AddRef()
{
    ++m_refCountAndRefreshStatus;
}

void ShaderAsset::Release()
{
    ASSERT((m_refCountAndRefreshStatus & REF_COUNT_MASK) > 0);
    --m_refCountAndRefreshStatus;
}

GpuShaderProgramID ShaderAsset::Refresh(const char* data, size_t length)
{
    GpuShaderProgramID old = m_shaderProgram;
    m_shaderProgram = m_device.ShaderProgramCreate(data, length);
    m_refCountAndRefreshStatus |= REFRESH_STATUS_MASK;
    return old;
}

bool ShaderAsset::PollRefreshed() const
{
    return (m_refCountAndRefreshStatus & REFRESH_STATUS_MASK);
}

void ShaderAsset::ClearRefreshedStatus()
{
    m_refCountAndRefreshStatus &= ~(REFRESH_STATUS_MASK);
}

ShaderCache::ShaderCache(GpuDevice& device, FileLoader& loader)
    : m_device(device)
    , m_fileLoader(loader)
    , m_shaders()
    , m_refreshQueue(&ShaderCache::RefreshPerform, &ShaderCache::RefreshFinalize, (void*)this)
{}

ShaderCache::~ShaderCache()
{
    m_refreshQueue.Clear();

    auto iter = m_shaders.begin();
    auto end = m_shaders.end();
    for ( ; iter != end; ++iter) {
        ASSERT(iter->second->RefCount() == 0);
        delete iter->second;
    }
}

static std::string GetPath(const char* name)
{
    return name + std::string("_MTL.shd");
}

static void* Alloc(u32 size, void*) { return malloc(size); }

ShaderAsset* ShaderCache::FindOrLoad(const char* name)
{
    std::string path(GetPath(name));

    auto iter = m_shaders.find(path);
    if (iter != m_shaders.end())
        return iter->second;

    u8* data;
    u32 size;
    m_fileLoader.Load(path.c_str(), &data, &size, Alloc, NULL);

    ShaderAsset* shader = new ShaderAsset(m_device, (const char*)data, (size_t)size);

    free(data);

    m_shaders[path] = shader;

    return shader;
}

void ShaderCache::Refresh(const char* name)
{
    std::string path(GetPath(name));

    auto iter = m_shaders.find(path);
    if (iter == m_shaders.end())
        return;
    ShaderAsset* shader = iter->second;

    shader->AddRef();
    m_refreshQueue.QueueRefresh(shader, path.c_str());
}

void ShaderCache::UpdateRefreshSystem()
{
    m_refreshQueue.Update();
}

GpuShaderProgramID ShaderCache::RefreshPerform(ShaderAsset* shader,
                                               const char* path,
                                               void* userdata)
{
    ShaderCache* self = (ShaderCache*)userdata;

    u8* data;
    u32 size;
    self->m_fileLoader.Load(path, &data, &size, Alloc, NULL);

    GpuShaderProgramID old = shader->Refresh((const char*)data, (size_t)size);

    free(data);

    return old;
}

void ShaderCache::RefreshFinalize(ShaderAsset* shader,
                                  GpuShaderProgramID oldProgram,
                                  void* userdata)
{
    ShaderCache* self = (ShaderCache*)userdata;

    if (oldProgram != GpuShaderProgramID())
        self->m_device.ShaderProgramDestroy(oldProgram);
    shader->ClearRefreshedStatus();
    shader->Release();
}
