#ifndef MATH_QUATERNION_H
#define MATH_QUATERNION_H

class Quaternion {
public:
    Quaternion();

    void Slerp(float t, const Quaternion& a, const Quaternion& b);
    Quaternion& operator+=(const Quaternion& rhs);
    Quaternion& operator*=(float f);

    float x;
    float y;
    float z;
    float w;
};

inline Quaternion operator+(const Quaternion& lhs, const Quaternion& rhs)
{
    return Quaternion(lhs) += rhs;
}

inline Quaternion operator*(const Quaternion& lhs, float rhs)
{
    return Quaternion(lhs) *= rhs;
}

inline Quaternion operator*(float lhs, const Quaternion& rhs)
{
    return Quaternion(rhs) *= lhs;
}

#endif // MATH_QUATERNION_H
