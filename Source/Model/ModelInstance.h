#ifndef MODEL_MODELINSTANCE_H
#define MODEL_MODELINSTANCE_H

#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuDrawItemPool.h"

class Matrix44;
class Vector3;
class ModelAsset;
class ModelScene;

// Create and destroy via ModelScene::CreateModelInstance()
// and ModelScene::DestroyModelInstance().
class ModelInstance {
public:
    enum Flags {
        FLAG_SKYBOX = 1,
        FLAG_WIREFRAME = 2,
    };

    ModelAsset* GetModelAsset() const;
    GpuBufferID GetCBuffer() const;

    void RefreshDrawItems();
    void AddDrawItemsToList(std::vector<const GpuDrawItem*>& items);

    u32 GetFlags() const;
    void SetFlags(u32 flags);

    void Update(const Matrix44& worldTransform,
                const Vector3& diffuseColor,
                const Vector3& specularColor,
                float glossiness);

private:
    friend class ModelScene;
    ModelInstance(const ModelInstance&);
    ModelInstance& operator=(const ModelInstance&);

    ModelInstance(ModelScene& scene, ModelAsset* model, u32 flags);
    ~ModelInstance();

    ModelScene& m_scene;
    ModelAsset* m_model;
    u32 m_flags;
    GpuBufferID m_cbuffer;
    GpuDrawItemPoolIndex m_drawItemIndex;
};

#endif // MODEL_MODELINSTANCE_H
