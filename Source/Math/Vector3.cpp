#include "Math/Vector3.h"
#include "Math/Vector4.h"

Vector3::Vector3()
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
{}

Vector3::Vector3(float xVal, float yVal, float zVal)
    : x(xVal)
    , y(yVal)
    , z(zVal)
{}

Vector3::Vector3(const Vector4& other)
    : x(other.x)
    , y(other.y)
    , z(other.z)
{}

Vector3& Vector3::operator=(const Vector3& rhs)
{
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
    return *this;
}

Vector3& Vector3::operator+=(const Vector3& rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

Vector3& Vector3::operator*=(float rhs)
{
    x *= rhs;
    y *= rhs;
    z *= rhs;
    return *this;
}

Vector3& Vector3::operator/=(float rhs)
{
    x /= rhs;
    y /= rhs;
    z /= rhs;
    return *this;
}

const Vector3 Vector3::operator-() const
{
    return Vector3(-x, -y, -z);
}

const Vector3 Normalize(const Vector3& vec)
{
    float len = SquaredLength(vec);
    if (len) {
        len = 1.0f / sqrtf(len);
        return Vector3(vec) *= len;
    }
    return vec;
}
