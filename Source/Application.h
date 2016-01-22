#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include "GpuDevice/GpuDevice.h"
#include "Asset/AssetCache.h"
#include "Model/ModelRenderQueue.h"
#include "Model/ModelInstance.h"

class OsWindow;

class Application {
public:
    Application();
    ~Application();
    void Frame();
private:
    Application(const Application&);
    Application& operator=(const Application&);

    OsWindow* m_window;
    GpuDevice* m_gpuDevice;
    GpuRenderPassID m_renderPass;
    AssetCache<ModelAsset>* m_modelCache;
    ModelRenderQueue* m_modelRenderQueue;
    ModelInstance* m_modelInstance;
    float m_angle;
};

#endif // APPLICATION_H
