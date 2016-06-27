#ifndef SCENE_SCENE_H
#define SCENE_SCENE_H

#include "Math/Matrix44.h"
#include "Model/ModelScene.h"
#include "Model/ModelRenderQueue.h"
#include "Scene/RenderTargetDisplay.h"

class GpuSamplerCache;
class ShaderCache;
template<class T> class AssetCache;

struct SceneUpdateInfo {
    Vector3 cameraPos;
    Vector3 forward;
    Vector3 right;
    Vector3 up;
    float zNear;
    float zFar;
    float fovY;
};

class Scene {
public:
    Scene(
        GpuDevice& device,
        FileLoader& loader,
        GpuSamplerCache& samplerCache,
        ShaderCache& shaderCache,
        TextureCache& textureCache
    );
    ~Scene();

    void SetSkybox(const char* path);
    ModelInstance* AddModelInstance(const char* path);
    void RefreshModel(const char* path);

    void Update(const SceneUpdateInfo& info);
    void Render(const GpuViewport& viewport);
private:
    Scene(const Scene&);
    Scene& operator=(const Scene&);

    GpuDevice& m_device;
    RenderTargetDisplay m_renderTargetDisplay;

    ModelScene m_modelScene;
    ModelRenderQueue m_modelRenderQueue;
    std::vector<ModelInstance*> m_modelInstances;
    ModelInstance* m_skybox;

    GpuTextureID m_colorRenderTarget;
    GpuTextureID m_depthRenderTarget;
    GpuRenderPassID m_renderPass;

    Vector3 m_cameraPos;
    Vector3 m_forward;
    Vector3 m_right;
    Vector3 m_up;
    float m_zNear;
    float m_zFar;
    float m_fovY;
};

#endif // SCENE_SCENE_H
