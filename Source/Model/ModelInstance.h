#ifndef MODEL_MODELINSTANCE_H
#define MODEL_MODELINSTANCE_H

#include <memory>
#include "GpuDevice/GpuDevice.h"

class ModelAsset;
class Matrix44;
class Vector3;

struct ModelInstanceCreateContext {
    std::shared_ptr<ModelAsset> model;
    GpuPipelineStateID pipelineObject;
    GpuBufferID sceneCBuffer;
    GpuTextureID defaultTexture;
    GpuSamplerID samplerNonMipmapped;
};

class ModelInstance {
public:
    static ModelInstance* Create(const ModelInstanceCreateContext& ctx);
    static void Destroy(ModelInstance* instance);

    const GpuDrawItem* GetDrawItem() const;
    ModelAsset* GetModelAsset() const;
    GpuBufferID GetCBuffer() const;

    void Update(const Matrix44& worldTransform,
                const Vector3& diffuseColor,
                const Vector3& specularColor,
                float glossiness);
private:
    ModelInstance(const ModelInstanceCreateContext& ctx);
    ~ModelInstance();
    ModelInstance(const ModelInstance&);
    ModelInstance& operator=(const ModelInstance&);

    std::shared_ptr<ModelAsset> m_model;
    GpuBufferID m_cbuffer;
};

#endif // MODEL_MODELINSTANCE_H
