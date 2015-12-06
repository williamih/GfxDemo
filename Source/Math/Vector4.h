#ifndef MATH_VECTOR4_H
#define MATH_VECTOR4_H

#include <math.h>

class Vector4 {
public:
    Vector4();
    Vector4(float xVal, float yVal, float zVal, float wVal);

    Vector4& operator=(const Vector4& rhs);
    Vector4& operator+=(const Vector4& rhs);
    Vector4& operator-=(const Vector4& rhs);
    Vector4& operator*=(float rhs);
    Vector4& operator/=(float rhs);

    const Vector4 operator-() const;

    float x;
    float y;
    float z;
    float w;
};

inline const Vector4 operator+(const Vector4& lhs, const Vector4& rhs)
{
    return Vector4(lhs) += rhs;
}

inline const Vector4 operator-(const Vector4& lhs, const Vector4& rhs)
{
    return Vector4(lhs) -= rhs;
}

inline const Vector4 operator*(const Vector4& lhs, float rhs)
{
    return Vector4(lhs) *= rhs;
}

inline const Vector4 operator*(float lhs, const Vector4& rhs)
{
    return Vector4(rhs) *= lhs;
}

inline const Vector4 operator/(const Vector4& lhs, float rhs)
{
    return Vector4(lhs) /= rhs;
}

inline float Dot(const Vector4& a, const Vector4& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float SquaredLength(const Vector4& vec)
{
    return Dot(vec, vec);
}

inline float Length(const Vector4& vec)
{
    return sqrtf(SquaredLength(vec));
}

const Vector4 Normalize(const Vector4& vec);

#endif // MATH_VECTOR4_H
