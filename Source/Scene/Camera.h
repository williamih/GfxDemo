#ifndef SCENE_CAMERA_H
#define SCENE_CAMERA_H

#include "Core/Types.h"
#include "Math/Vector3.h"
#include "Math/Matrix33.h"
#include "Math/Matrix44.h"

class Camera {
public:
    Camera();

    void MoveForwardStart();
    void MoveForwardStop();
    void MoveBackwardStart();
    void MoveBackwardStop();

    void AscendStart();
    void AscendStop();
    void DescendStart();
    void DescendStop();

    void TurnLeftStart();
    void TurnLeftStop();
    void TurnRightStart();
    void TurnRightStop();

    void SetPositionAndTarget(const Vector3& position, const Vector3& target);

    void HandleMouseDrag(float normalizedDeltaX, float normalizedDeltaY);

    void Update(float dt);

    Vector3 Position() const;
    Vector3 Forward() const;
    Vector3 Right() const;
    Vector3 Up() const;

    float ZNear() const;
    float ZFar() const;
    float FOV() const;
private:
    Camera(const Camera&);
    Camera& operator=(const Camera&);

    Vector3 m_targetPos;
    float m_targetYaw;
    float m_distToTarget;
    float m_yaw;
    float m_pitch;
    Matrix33 m_rotation;
    u32 m_moveFlags;
};

#endif // SCENE_CAMERA_H
