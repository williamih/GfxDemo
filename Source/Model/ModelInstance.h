#ifndef MODEL_MODELINSTANCE_H
#define MODEL_MODELINSTANCE_H

#include "Core/List.h"
#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuDrawItemPool.h"

class Matrix44;
class Vector3;
class ModelShared;
class ModelScene;

// Create and destroy via ModelScene::CreateModelInstance()
// and ModelScene::DestroyModelInstance().
class ModelInstance {
public:
    enum Flags {
        FLAG_SKYBOX = 1,
        FLAG_WIREFRAME = 2,
    };

    ModelShared* GetShared() const;
    GpuBufferID GetCBuffer() const;

    u32 GetFlags() const;
    void SetFlags(u32 flags);

    void Update(const Matrix44& worldTransform,
                const Vector3& diffuseColor,
                const Vector3& specularColor,
                float glossiness);

    void RecreateDrawItems();
    void Reload(ModelShared* newShared);
    void AddDrawItemsToList(std::vector<const GpuDrawItem*>& items);

    ModelInstance* NextInAssetGroup();
    void MarkLastInAssetGroup();

    LIST_LINK(ModelInstance) m_link;

private:
    friend class ModelScene;
    ModelInstance(const ModelInstance&);
    ModelInstance& operator=(const ModelInstance&);

    ModelInstance(ModelScene& scene, ModelShared* model, u32 flags);
    ~ModelInstance();

    ModelScene& m_scene;
    ModelShared* m_shared;
    u32 m_flagsAndAssetGroupInfo;
    GpuBufferID m_cbuffer;
    GpuDrawItemPoolIndex m_drawItemIndex;
};

#endif // MODEL_MODELINSTANCE_H
