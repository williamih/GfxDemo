#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include "GpuDevice/GpuDevice.h"
#include "Asset/AssetCache.h"
#include "Model/ModelRenderQueue.h"
#include "Model/ModelInstance.h"

class OsWindow;
class ShaderAsset;
class ShaderAssetFactory;
class ModelAssetFactory;
class GpuDeferredDeletionQueue;

class Application {
public:
    Application();
    ~Application();
    void Frame();
    void RefreshModelShader();
private:
    Application(const Application&);
    Application& operator=(const Application&);

    OsWindow* m_window;
    GpuDevice* m_gpuDevice;
    GpuRenderPassID m_renderPass;
#ifdef ASSET_REFRESH
    GpuDeferredDeletionQueue* m_gpuDeferredDeletionQueue;
#endif
    ShaderAssetFactory* m_shaderAssetFactory;
    ModelAssetFactory* m_modelAssetFactory;
    AssetCache<ModelAsset>* m_modelCache;
    AssetCache<ShaderAsset>* m_shaderCache;
    ModelRenderQueue* m_modelRenderQueue;
    ModelInstance* m_modelInstance;
    float m_angle;
};

#endif // APPLICATION_H
