#include "Shader/ShaderAsset.h"

#include <stdlib.h>

#include "Core/Macros.h"
#include "Core/Str.h"
#include "Core/FileLoader.h"

namespace {
    struct GetShaderKey {
        HashKey_Str operator()(ShaderAsset* shader) const
        {
            HashKey_Str key;
            key.str = shader->GetName();
            return key;
        }
    };
}

const int REFRESH_STATUS_MASK = 0x80000000;
const int REF_COUNT_MASK = ~(REFRESH_STATUS_MASK);

const int SHADER_MAX_PATH_LENGTH = 260;

static void* Alloc(u32 size, void*) { return malloc(size); }

static void PrintFullPath(char* dst, size_t dstChars, const char* shaderName)
{
    StrPrintf(dst, dstChars, "%s_MTL.shd", shaderName);
}

ShaderAsset::ShaderAsset(GpuDevice& device, FileLoader& loader, const char* name)
    : m_link()

    , m_device(device)
    , m_shaderProgram(0)
    , m_refCountAndRefreshStatus(0)
{
    char fullPath[SHADER_MAX_PATH_LENGTH];
    PrintFullPath(fullPath, sizeof fullPath, name);

    u8* data;
    u32 size;
    loader.Load(fullPath, &data, &size, Alloc, NULL);

    m_shaderProgram = device.ShaderProgramCreate((const char*)data, (size_t)size);

    free(data);
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

const char* ShaderAsset::GetName() const
{
    return (const char*)(this + 1);
}

GpuShaderProgramID ShaderAsset::Refresh(FileLoader& loader)
{
    GpuShaderProgramID old = m_shaderProgram;

    char fullPath[SHADER_MAX_PATH_LENGTH];
    PrintFullPath(fullPath, sizeof fullPath, GetName());

    u8* data;
    u32 size;
    loader.Load(fullPath, &data, &size, Alloc, NULL);

    m_shaderProgram = m_device.ShaderProgramCreate((const char*)data, (size_t)size);

    free(data);

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

ShaderAsset* ShaderAsset::Create(GpuDevice& device, FileLoader& loader,
                                 const char* path)
{
    size_t pathLen = StrLen(path) + 1; // include null terminator ( + 1 )
    void* memory = malloc(sizeof(ShaderAsset) + pathLen);

    // Copy over the path
    memcpy((u8*)memory + sizeof(ShaderAsset), path, pathLen);

    // Create the shader
    return new (memory) ShaderAsset(device, loader, path);
}

void ShaderAsset::Destroy(ShaderAsset* shader)
{
    shader->~ShaderAsset();
    free(shader);
}

ShaderCache::ShaderCache(GpuDevice& device, FileLoader& loader)
    : m_device(device)
    , m_fileLoader(loader)

    , m_shaderList()
    , m_shaderHash()

    , m_refreshQueue(&ShaderCache::RefreshPerform,
                     &ShaderCache::RefreshFinalize, (void*)this)
{}

ShaderCache::~ShaderCache()
{
    m_refreshQueue.Clear();

    RemoveUnusedShaders();
    ASSERT(m_shaderList.Head() == NULL);
    ASSERT(m_shaderHash.Count() == 0);
}

ShaderAsset* ShaderCache::FindOrLoad(const char* name)
{
    HashKey_Str key;
    key.str = name;

    if (ShaderAsset* const* ppShader = m_shaderHash.Get(key, GetShaderKey()))
        return *ppShader;

    ShaderAsset* shader = ShaderAsset::Create(m_device, m_fileLoader, name);

    m_shaderList.InsertTail(shader);
    m_shaderHash.Insert(shader, GetShaderKey());

    return shader;
}

void ShaderCache::RemoveUnusedShaders()
{
    for (ShaderAsset* shader = m_shaderList.Head(); shader; ) {
        ShaderAsset* next = shader->m_link.Next();

        if (shader->RefCount() == 0) {
            HashKey_Str key;
            key.str = shader->GetName();
            ASSERT(m_shaderHash.Delete(key, GetShaderKey()));
            ShaderAsset::Destroy(shader);
        }

        shader = next;
    }
}

void ShaderCache::Refresh(const char* name)
{
    HashKey_Str key;
    key.str = name;

    ShaderAsset* const* ppShader = m_shaderHash.Get(key, GetShaderKey());
    if (!ppShader)
        return;

    ShaderAsset* shader = *ppShader;

    shader->AddRef();
    m_refreshQueue.QueueRefresh(shader);
}

void ShaderCache::UpdateRefreshSystem()
{
    m_refreshQueue.Update();
}

GpuShaderProgramID ShaderCache::RefreshPerform(ShaderAsset* shader,
                                               void* userdata)
{
    ShaderCache* self = (ShaderCache*)userdata;

    GpuShaderProgramID old = shader->Refresh(self->m_fileLoader);

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
