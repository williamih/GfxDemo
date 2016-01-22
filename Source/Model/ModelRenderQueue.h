#ifndef MODEL_MODELRENDERQUEUE_H
#define MODEL_MODELRENDERQUEUE_H

#include <vector>
#include "Math/Vector3.h"
#include "Math/Matrix44.h"
#include "GpuDevice/GpuDevice.h"

class ModelAsset;
class ModelInstance;

class ModelRenderQueue {
public:
    struct SceneInfo {
        Matrix44 viewProjTransform;
        Vector3 cameraPos;
        Vector3 dirToLight;
        Vector3 irradiance;
        Vector3 ambientRadiance;
    };

    explicit ModelRenderQueue(GpuDevice* device);
    ~ModelRenderQueue();

    ModelInstance* CreateModelInstance(std::shared_ptr<ModelAsset> model);

    void Clear();
    void Add(ModelInstance* instance);
    void Draw(const SceneInfo& sceneInfo,
              const GpuViewport& viewport,
              GpuRenderPassID renderPass);
private:
    ModelRenderQueue(const ModelRenderQueue&);
    ModelRenderQueue& operator=(const ModelRenderQueue&);

    std::vector<ModelInstance*> m_modelInstances;
    std::vector<const GpuDrawItem*> m_drawItems;
    GpuDevice* m_device;
    GpuBufferID m_sceneCBuffer;
    GpuShaderProgramID m_shaderProgram;
    GpuTextureID m_defaultTexture;
    GpuSamplerID m_sampler;
    GpuInputLayoutID m_inputLayout;
    GpuPipelineStateID m_pipelineStateObj;
};

#endif // MODEL_MODELRENDERQUEUE_H
