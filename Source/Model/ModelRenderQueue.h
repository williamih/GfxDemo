#ifndef MODEL_MODELRENDERQUEUE_H
#define MODEL_MODELRENDERQUEUE_H

#include <vector>
#include "Math/Vector3.h"
#include "Math/Matrix44.h"
#include "GpuDevice/GpuDevice.h"

class ShaderAsset;
class ModelAsset;
class ModelInstance;
class ModelScene;

class ModelRenderQueue {
public:
    struct SceneInfo {
        Matrix44 viewProjTransform;
        Vector3 cameraPos;
        Vector3 dirToLight;
        Vector3 irradiance;
        Vector3 ambientRadiance;
    };

    ModelRenderQueue();

    void Clear();
    void Add(ModelInstance* instance);
    void Draw(ModelScene& scene,
              const SceneInfo& sceneInfo,
              const GpuViewport& viewport,
              GpuRenderPassID renderPass);
private:
    ModelRenderQueue(const ModelRenderQueue&);
    ModelRenderQueue& operator=(const ModelRenderQueue&);

    std::vector<const GpuDrawItem*> m_drawItems;
};

#endif // MODEL_MODELRENDERQUEUE_H
