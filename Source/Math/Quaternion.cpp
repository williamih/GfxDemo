#include "Math/Quaternion.h"
#include <math.h>
#include "Core/Macros.h"

Quaternion::Quaternion()
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
    , w(1.0f)
{}

void Quaternion::Slerp(float t, const Quaternion& a, const Quaternion& b)
{
    float w0 = a.w;
    float x0 = a.x;
    float y0 = a.y;
    float z0 = a.z;
    float w1 = b.w;
    float x1 = b.x;
    float y1 = b.y;
    float z1 = b.z;

    float cosOmega = w0 * w1 + x0 * x1 + y0 * y1 + z0 * z1;

    if (cosOmega < 0.0f) {
        w1 = -w1;
        x1 = -x1;
        y1 = -y1;
        z1 = -z1;
        cosOmega = -cosOmega;
    }

    float k0, k1;
    if (cosOmega > 0.9999f) {
        k0 = 1.0f - t;
        k1 = t;
    } else {
        float sinOmega = sqrtf(1.0f - cosOmega * cosOmega);
        float omega = atan2f(sinOmega, cosOmega);
        float oneOverSinOmega = 1.0f / sinOmega;
        k0 = sinf((1.0f - t) * omega) * oneOverSinOmega;
        k1 = sinf(t * omega) * oneOverSinOmega;
    }

    w = w0 * k0 + w1 * k1;
    x = x0 * k0 + x1 * k1;
    y = y0 * k0 + y1 * k1;
    z = z0 * k0 + z1 * k1;
}

Quaternion& Quaternion::operator+=(const Quaternion& rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    w += rhs.w;
    return *this;
}

Quaternion& Quaternion::operator*=(float f)
{
    x *= f;
    y *= f;
    z *= f;
    w *= f;
    return *this;
}
