#ifndef MODEL_MODELINSTANCE_H
#define MODEL_MODELINSTANCE_H

#include <memory>
#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuDrawItemPool.h"

class Matrix44;
class Vector3;
class ModelAsset;

struct ModelInstanceCreateContext {
    GpuDrawItemPool* drawItemPool;
    GpuPipelineStateID pipelineObject;
    GpuBufferID sceneCBuffer;
    GpuTextureID defaultTexture;
    GpuSamplerID sampler;
};

class ModelInstance {
public:
    ModelInstance(std::shared_ptr<ModelAsset> model,
                  const ModelInstanceCreateContext& ctx);
    ~ModelInstance();

    ModelAsset* GetModelAsset() const;
    GpuBufferID GetCBuffer() const;

    void RefreshDrawItem(const ModelInstanceCreateContext& ctx);
    void AddDrawItemsToList(std::vector<const GpuDrawItem*>& items);

    void Update(const Matrix44& worldTransform,
                const Vector3& diffuseColor,
                const Vector3& specularColor,
                float glossiness);
private:
    ModelInstance(const ModelInstance&);
    ModelInstance& operator=(const ModelInstance&);

    GpuDrawItemPool& m_drawItemPool;
    std::shared_ptr<ModelAsset> m_model;
    GpuBufferID m_cbuffer;
    GpuDrawItemPoolIndex m_drawItemIndex;
};

#endif // MODEL_MODELINSTANCE_H
