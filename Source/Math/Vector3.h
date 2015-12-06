#ifndef MATH_VECTOR3_H
#define MATH_VECTOR3_H

#include <math.h>

class Vector4;

class Vector3 {
public:
    Vector3();
    Vector3(float xVal, float yVal, float zVal);
    Vector3(const Vector4& other);

    Vector3& operator=(const Vector3& rhs);
    Vector3& operator+=(const Vector3& rhs);
    Vector3& operator-=(const Vector3& rhs);
    Vector3& operator*=(float rhs);
    Vector3& operator/=(float rhs);

    const Vector3 operator-() const;

    float x;
    float y;
    float z;
};

inline const Vector3 operator+(const Vector3& lhs, const Vector3& rhs)
{
    return Vector3(lhs) += rhs;
}

inline const Vector3 operator-(const Vector3& lhs, const Vector3& rhs)
{
    return Vector3(lhs) -= rhs;
}

inline const Vector3 operator*(const Vector3& lhs, float rhs)
{
    return Vector3(lhs) *= rhs;
}

inline const Vector3 operator*(float lhs, const Vector3& rhs)
{
    return Vector3(rhs) *= lhs;
}

inline const Vector3 operator/(const Vector3& lhs, float rhs)
{
    return Vector3(lhs) /= rhs;
}

inline float Dot(const Vector3& a, const Vector3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline const Vector3 Cross(const Vector3& a, const Vector3& b)
{
    return Vector3(a.y * b.z - a.z * b.y,
                   a.z * b.x - a.x * b.z,
                   a.x * b.y - a.y * b.x);
}

inline float SquaredLength(const Vector3& vec)
{
    return Dot(vec, vec);
}

inline float Length(const Vector3& vec)
{
    return sqrtf(SquaredLength(vec));
}

const Vector3 Normalize(const Vector3& vec);

#endif // MATH_VECTOR3_H
