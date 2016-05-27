#include "Math/Vector4.h"

#include "Math/Vector3.h"

Vector4::Vector4()
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
    , w(0.0f)
{}

Vector4::Vector4(float xVal, float yVal, float zVal, float wVal)
    : x(xVal)
    , y(yVal)
    , z(zVal)
    , w(wVal)
{}

Vector4::Vector4(const Vector3& vec, float wVal)
    : x(vec.x)
    , y(vec.y)
    , z(vec.z)
    , w(wVal)
{}

Vector4& Vector4::operator=(const Vector4& rhs)
{
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
    w = rhs.w;
    return *this;
}

Vector4& Vector4::operator+=(const Vector4& rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    w += rhs.w;
    return *this;
}

Vector4& Vector4::operator-=(const Vector4& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    w -= rhs.w;
    return *this;
}

Vector4& Vector4::operator*=(float rhs)
{
    x *= rhs;
    y *= rhs;
    z *= rhs;
    w *= rhs;
    return *this;
}

Vector4& Vector4::operator/=(float rhs)
{
    x /= rhs;
    y /= rhs;
    z /= rhs;
    w /= rhs;
    return *this;
}

const Vector4 Vector4::operator-() const
{
    return Vector4(-x, -y, -z, -w);
}

const Vector4 Normalize(const Vector4& vec)
{
    float len = SquaredLength(vec);
    if (len) {
        len = 1.0f / sqrtf(len);
        return Vector4(vec) *= len;
    }
    return vec;
}
