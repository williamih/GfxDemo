#ifndef MODEL_MODELSCENE_H
#define MODEL_MODELSCENE_H

#include <vector>
#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuDrawItemPool.h"

template<class T> class AssetCache;

class ShaderAsset;
class ModelAsset;
class ModelInstance;

class ModelScene {
public:
    struct SceneCBuffer {
        float viewProjTransform[4][4];
        float cameraPos[4];
        float dirToLight[4];
        float irradiance_over_pi[4];
        float ambientRadiance[4];
    };

    ModelScene(GpuDevice* device, AssetCache<ShaderAsset>& shaderCache);
    ~ModelScene();

    ModelInstance* CreateModelInstance(ModelAsset* model, u32 flags);

    void SetMaxAnisotropy(int maxAnisotropy);

    GpuDevice* GetGpuDevice() const;
    GpuBufferID GetSceneCBuffer() const;

    void Update();
private:
    ModelScene(const ModelScene&);
    ModelScene& operator=(const ModelScene&);

    GpuPipelineStateID CreateModelPSO();
    GpuPipelineStateID CreateSkyboxPSO();
    void RefreshModelPSO();
    void RefreshSkyboxPSO();

    std::vector<ModelInstance*> m_modelInstances;
    GpuDevice* m_device;
    GpuDrawItemPool m_drawItemPool;
    ShaderAsset* m_modelShader;
    ShaderAsset* m_skyboxShader;
    GpuBufferID m_sceneCBuffer;
    GpuTextureID m_defaultTexture;
    GpuSamplerID m_samplerUVClamp;
    GpuSamplerID m_samplerUVRepeat;
    GpuInputLayoutID m_inputLayout;
    GpuPipelineStateID m_modelPSO;
    GpuPipelineStateID m_skyboxPSO;
};

#endif // MODEL_MODELSCENE_H
