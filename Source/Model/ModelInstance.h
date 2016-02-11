#ifndef MODEL_MODELINSTANCE_H
#define MODEL_MODELINSTANCE_H

#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuDrawItemPool.h"

class Matrix44;
class Vector3;
class ModelAsset;

struct ModelInstanceCreateContext {
    GpuDrawItemPool* drawItemPool;
    GpuBufferID sceneCBuffer;
    GpuTextureID defaultTexture;
    GpuSamplerID samplerUVClamp;
    GpuSamplerID samplerUVRepeat;

    GpuPipelineStateID modelPSO;
    GpuPipelineStateID skyboxPSO;
};

class ModelInstance {
public:
    enum Flags {
        FLAG_SKYBOX = 1,
    };

    ModelInstance(ModelAsset* model,
                  u32 flags,
                  const ModelInstanceCreateContext& ctx);
    ~ModelInstance();

    ModelAsset* GetModelAsset() const;
    GpuBufferID GetCBuffer() const;

    void RefreshDrawItems(const ModelInstanceCreateContext& ctx);
    void AddDrawItemsToList(std::vector<const GpuDrawItem*>& items);

    void Update(const Matrix44& worldTransform,
                const Vector3& diffuseColor,
                const Vector3& specularColor,
                float glossiness);
private:
    ModelInstance(const ModelInstance&);
    ModelInstance& operator=(const ModelInstance&);

    GpuDrawItemPool& m_drawItemPool;
    ModelAsset* m_model;
    u32 m_flags;
    GpuBufferID m_cbuffer;
    GpuDrawItemPoolIndex m_drawItemIndex;
};

#endif // MODEL_MODELINSTANCE_H
