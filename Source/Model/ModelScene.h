#ifndef MODEL_MODELSCENE_H
#define MODEL_MODELSCENE_H

#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuDrawItemPool.h"
#include "Model/ModelInstance.h"
#include "Model/ModelCache.h"

class FileLoader;
class GpuSamplerCache;
class ShaderAsset;
class ShaderCache;
class TextureCache;
class ModelInstance;

class ModelScene {
public:
    enum PSOFlag {
        PSOFLAG_SKYBOX = 1 << 0,
        PSOFLAG_WIREFRAME = 1 << 1,

        PSOFLAG_NUMPERMUTATIONS = 1 << 2,
    };

    struct SceneCBuffer {
        float viewProjTransform[4][4];
        float cameraPos[4];
        float dirToLight[4];
        float irradiance_over_pi[4];
        float ambientRadiance[4];
    };

    ModelScene(
        GpuDevice& device,
        FileLoader& loader,
        GpuSamplerCache& samplerCache,
        ShaderCache& shaderCache,
        TextureCache& textureCache
    );
    ~ModelScene();

    ModelInstance* CreateModelInstance(const char* path, u32 flags);
    void DestroyModelInstance(ModelInstance* instance);
    void Reload(const char* path);

    void Update();

    GpuPipelineStateID RequestPSO(u32 flags);
    GpuSamplerID GetSamplerUVClamp() const;
    GpuSamplerID GetSamplerUVRepeat() const;
    GpuTextureID GetDefaultTexture() const;
    GpuDrawItemPool& GetDrawItemPool();
    GpuBufferID GetSceneCBuffer() const;
    GpuDevice& GetGpuDevice() const;
private:
    ModelScene(const ModelScene&);
    ModelScene& operator=(const ModelScene&);

    static void SamplerCacheCallback(GpuSamplerCache& cache, void* userdata);
    void RefreshPSOsMatching(u32, u32);

    GpuDevice& m_device;
    FileLoader& m_fileLoader;
    GpuSamplerCache& m_samplerCache;
    TextureCache& m_textureCache;

    LIST_DECLARE(ModelInstance, m_link) m_modelInstances;
    ModelCache m_modelCache;

    GpuDrawItemPool m_drawItemPool;

    ShaderAsset* m_modelShader;
    ShaderAsset* m_skyboxShader;

    GpuBufferID m_sceneCBuffer;
    GpuTextureID m_defaultTexture;
    GpuSamplerID m_samplerUVClamp;
    GpuSamplerID m_samplerUVRepeat;
    GpuInputLayoutID m_inputLayout;
    GpuPipelineStateID m_PSOs[PSOFLAG_NUMPERMUTATIONS];
};

#endif // MODEL_MODELSCENE_H
