#ifndef MODEL_MODELRENDERQUEUE_H
#define MODEL_MODELRENDERQUEUE_H

#include <vector>
#include "Math/Vector3.h"
#include "Math/Matrix44.h"
#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuDrawItemPool.h"
#include "Asset/AssetCache.h"

class ModelAsset;
class ModelInstance;
class ShaderAsset;

class ModelRenderQueue {
public:
    struct SceneInfo {
        Matrix44 viewProjTransform;
        Vector3 cameraPos;
        Vector3 dirToLight;
        Vector3 irradiance;
        Vector3 ambientRadiance;
    };

    ModelRenderQueue(GpuDevice* device, AssetCache<ShaderAsset>& shaderCache);
    ~ModelRenderQueue();

    ModelInstance* CreateModelInstance(ModelAsset* model);

    void SetMaxAnisotropy(int maxAnisotropy);

    void Clear();
    void Add(ModelInstance* instance);
    void Draw(const SceneInfo& sceneInfo,
              const GpuViewport& viewport,
              GpuRenderPassID renderPass);
private:
    ModelRenderQueue(const ModelRenderQueue&);
    ModelRenderQueue& operator=(const ModelRenderQueue&);

    void RefreshPipelineStateObject();

    std::vector<const GpuDrawItem*> m_drawItems;
    std::vector<ModelInstance*> m_modelInstances;
    GpuDevice* m_device;
    GpuDrawItemPool m_drawItemPool;
    ShaderAsset* m_shaderAsset;
    GpuBufferID m_sceneCBuffer;
    GpuTextureID m_defaultTexture;
    GpuSamplerID m_sampler;
    GpuInputLayoutID m_inputLayout;
    GpuPipelineStateID m_pipelineStateObj;
};

#endif // MODEL_MODELRENDERQUEUE_H
