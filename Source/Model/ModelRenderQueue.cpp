#include "Model/ModelRenderQueue.h"

#include "Core/Macros.h"

#include "GpuDevice/GpuMathUtils.h"

#include "Model/ModelInstance.h"
#include "Model/ModelScene.h"

const float PI = 3.141592654f;

ModelRenderQueue::ModelRenderQueue()
    : m_drawItems()
{}

void ModelRenderQueue::Clear()
{
    m_drawItems.clear();
}

void ModelRenderQueue::Add(ModelInstance* instance)
{
    instance->AddDrawItemsToList(m_drawItems);
}

void ModelRenderQueue::Draw(ModelScene& scene,
                            const SceneInfo& sceneInfo,
                            const GpuViewport& viewport,
                            GpuRenderPassID renderPass)
{
    GpuDevice& device = scene.GetGpuDevice();
    GpuBufferID sceneCBuffer = scene.GetSceneCBuffer();

    ModelScene::SceneCBuffer* sceneCBuf;
    sceneCBuf = (ModelScene::SceneCBuffer*)device.BufferMap(sceneCBuffer);

    GpuMathUtils::FillArrayColumnMajor(sceneInfo.viewProjTransform,
                                       sceneCBuf->viewProjTransform);
    sceneCBuf->cameraPos[0] = sceneInfo.cameraPos.x;
    sceneCBuf->cameraPos[1] = sceneInfo.cameraPos.y;
    sceneCBuf->cameraPos[2] = sceneInfo.cameraPos.z;
    sceneCBuf->cameraPos[3] = 0.0f;
    sceneCBuf->dirToLight[0] = sceneInfo.dirToLight.x;
    sceneCBuf->dirToLight[1] = sceneInfo.dirToLight.y;
    sceneCBuf->dirToLight[2] = sceneInfo.dirToLight.z;
    sceneCBuf->dirToLight[3] = 0.0f;
    sceneCBuf->irradiance_over_pi[0] = sceneInfo.irradiance.x / PI;
    sceneCBuf->irradiance_over_pi[1] = sceneInfo.irradiance.y / PI;
    sceneCBuf->irradiance_over_pi[2] = sceneInfo.irradiance.z / PI;
    sceneCBuf->irradiance_over_pi[3] = 0.0f;
    sceneCBuf->ambientRadiance[0] = sceneInfo.ambientRadiance.x;
    sceneCBuf->ambientRadiance[1] = sceneInfo.ambientRadiance.y;
    sceneCBuf->ambientRadiance[2] = sceneInfo.ambientRadiance.z;
    sceneCBuf->ambientRadiance[3] = 0.0f;

    device.BufferUnmap(sceneCBuffer);

    device.Draw(&m_drawItems[0], (int)m_drawItems.size(), renderPass, viewport);
}
