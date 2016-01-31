#include "Scene/Camera.h"

#include <math.h>

#include "GpuDevice/GpuDevice.h"

const float PI      = 3.141592654f;
const float TWOPI   = 6.283185307f;
const float HALFPI  = 1.570796327f;

const float DEGTORAD = PI / 180.0f;

const float FOV_Y = 75.0f * DEGTORAD;
const float ASPECT = 4.0f/3.0f;
const float ZNEAR = 0.1f;
const float ZFAR = 50.0f;

const float HORIZ_SPEED = 2.0f; // length / sec
const float VERT_SPEED = 2.0f; // length / sec
const float ANGULAR_SPEED = 2.0f; // radians / sec

const float MOUSE_YAW_FACTOR = 3.0f;
const float MOUSE_PITCH_FACTOR = 3.0f;

const u32 MOVE_FLAG_FORWARD = 1;
const u32 MOVE_FLAG_BACKWARD = 2;
const u32 MOVE_FLAG_ASCEND = 4;
const u32 MOVE_FLAG_DESCEND = 8;
const u32 MOVE_FLAG_TURN_LEFT = 16;
const u32 MOVE_FLAG_TURN_RIGHT = 32;

static Matrix44 CreatePerspectiveMatrix(float aspect, float fovY, float zNear, float zFar)
{
    float tanHalfFovY = tanf(fovY * 0.5f);
    float top = tanHalfFovY * zNear;
    float bot = -top;
    float right = top * aspect;
    float left = -right;
    return GpuDevice::TransformCreatePerspective(left, right, bot, top, zNear, zFar);
}

Camera::Camera()
    : m_targetPos(0.0f, 0.0f, 0.0f)
    , m_targetYaw(0.0f)
    , m_distToTarget(0.0f)
    , m_yaw(0.0f)
    , m_pitch(0.0f)
    , m_rotation()
    , m_viewTransform()
    , m_projTransform(CreatePerspectiveMatrix(ASPECT, FOV_Y, ZNEAR, ZFAR))
    , m_moveFlags(0)
{}

void Camera::MoveForwardStart()
{
    m_moveFlags |= MOVE_FLAG_FORWARD;
}

void Camera::MoveForwardStop()
{
    m_moveFlags &= ~(MOVE_FLAG_FORWARD);
}

void Camera::MoveBackwardStart()
{
    m_moveFlags |= MOVE_FLAG_BACKWARD;
}

void Camera::MoveBackwardStop()
{
    m_moveFlags &= ~(MOVE_FLAG_BACKWARD);
}

void Camera::AscendStart()
{
    m_moveFlags |= MOVE_FLAG_ASCEND;
}

void Camera::AscendStop()
{
    m_moveFlags &= ~(MOVE_FLAG_ASCEND);
}

void Camera::DescendStart()
{
    m_moveFlags |= MOVE_FLAG_DESCEND;
}

void Camera::DescendStop()
{
    m_moveFlags &= ~(MOVE_FLAG_DESCEND);
}

void Camera::TurnLeftStart()
{
    m_moveFlags |= MOVE_FLAG_TURN_LEFT;
}

void Camera::TurnLeftStop()
{
    m_moveFlags &= ~(MOVE_FLAG_TURN_LEFT);
}

void Camera::TurnRightStart()
{
    m_moveFlags |= MOVE_FLAG_TURN_RIGHT;
}

void Camera::TurnRightStop()
{
    m_moveFlags &= ~(MOVE_FLAG_TURN_RIGHT);
}

void Camera::SetPositionAndTarget(const Vector3& position, const Vector3& target)
{
    Vector3 v = target - position;
    m_targetPos = target;
    m_distToTarget = Length(v);
    m_targetYaw = m_yaw = atan2f(-v.x, -v.z);
    m_pitch = asinf(v.y / m_distToTarget);
}

static Matrix33 RotateX(float theta)
{
    float sinTheta = sinf(theta);
    float cosTheta = cosf(theta);
    return Matrix33(1.0f, 0.0f,      0.0f,
                    0.0f, cosTheta, -sinTheta,
                    0.0f, sinTheta,  cosTheta);
}

static Matrix33 RotateY(float theta)
{
    float sinTheta = sinf(theta);
    float cosTheta = cosf(theta);
    return Matrix33(cosTheta,  0.0f,  sinTheta,
                    0.0f,      1.0f,  0.0f,
                    -sinTheta, 0.0f,  cosTheta);
}

void Camera::Update(float dt)
{
    // Update the target yaw, if we are turning left or right
    if ((m_moveFlags & MOVE_FLAG_TURN_LEFT) && !(m_moveFlags & MOVE_FLAG_TURN_RIGHT))
        m_targetYaw += ANGULAR_SPEED * dt;
    if ((m_moveFlags & MOVE_FLAG_TURN_RIGHT) && !(m_moveFlags & MOVE_FLAG_TURN_LEFT))
        m_targetYaw -= ANGULAR_SPEED * dt;

    // Wrap the yaw around if needed, so that it always lies in [-PI/2, +PI/2].
    // (This has no visual effect; it just keeps the angle within the canonical
    // range.)
    if (m_targetYaw > PI) m_targetYaw -= TWOPI;
    else if (m_targetYaw < -PI) m_targetYaw += TWOPI;

    // Update our horizontal (linear) motion, if we are moving forward or backward.
    Vector3 targetForward(-sinf(m_targetYaw), 0.0f, -cosf(m_targetYaw));
    if ((m_moveFlags & MOVE_FLAG_FORWARD) && !(m_moveFlags & MOVE_FLAG_BACKWARD))
        m_targetPos += targetForward * HORIZ_SPEED * dt;
    if ((m_moveFlags & MOVE_FLAG_BACKWARD) && !(m_moveFlags & MOVE_FLAG_FORWARD))
        m_targetPos -= targetForward * HORIZ_SPEED * dt;

    // Update our vertical motion, if we are ascending or descending.
    if ((m_moveFlags & MOVE_FLAG_ASCEND) && !(m_moveFlags & MOVE_FLAG_DESCEND))
        m_targetPos.y += VERT_SPEED * dt;
    if ((m_moveFlags & MOVE_FLAG_DESCEND) && !(m_moveFlags & MOVE_FLAG_ASCEND))
        m_targetPos.y -= VERT_SPEED * dt;

    // If we're moving at all, we lock the camera's actual yaw to our target
    // yaw value. This means that attempting to adjust the camera's yaw by
    // dragging the mouse has no effect.
    if (m_moveFlags)
        m_yaw = m_targetYaw;

    // Update our rotation matrix and world-to-camera (view) transform.
    m_rotation = RotateY(m_yaw) * RotateX(m_pitch);
    Vector3 pos = Position();
    Matrix44 cameraMatrix(m_rotation.m11, m_rotation.m12, m_rotation.m13, pos.x,
                          m_rotation.m21, m_rotation.m22, m_rotation.m23, pos.y,
                          m_rotation.m31, m_rotation.m32, m_rotation.m33, pos.z,
                          0.0f, 0.0f, 0.0f, 1.0f);
    m_viewTransform = cameraMatrix.AffineInverse();
}

void Camera::HandleMouseDrag(float normalizedDeltaX, float normalizedDeltaY)
{
    m_yaw -= MOUSE_YAW_FACTOR * normalizedDeltaX;
    if (m_yaw > PI) m_yaw -= TWOPI;
    if (m_yaw < -PI) m_yaw += TWOPI;

    m_pitch += MOUSE_PITCH_FACTOR * normalizedDeltaY;
    if (m_pitch > HALFPI) m_pitch = HALFPI;
    if (m_pitch < -HALFPI) m_pitch = -HALFPI;
}

Vector3 Camera::Position() const
{
    Vector3 targetToCamera(m_rotation.m13, m_rotation.m23, m_rotation.m33);
    return m_targetPos + targetToCamera * m_distToTarget;
}

Matrix44 Camera::ViewTransform() const
{
    return m_viewTransform;
}

Matrix44 Camera::ProjTransform() const
{
    return m_projTransform;
}
