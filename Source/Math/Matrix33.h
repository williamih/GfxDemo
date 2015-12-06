#ifndef MATH_MATRIX33_H
#define MATH_MATRIX33_H

#include "Math/Vector3.h"

class Matrix33 {
public:
    Matrix33();

    Matrix33(float a11, float a12, float a13,
             float a21, float a22, float a23,
             float a31, float a32, float a33);

    Matrix33(const Matrix33& other);

    Matrix33& operator=(const Matrix33& other);

    static Matrix33 FromEulerAnglesZXY(float z, float x, float y);

    const Matrix33 Inverse() const;
    const Matrix33 Transpose() const;

    float m11;
    float m12;
    float m13;
    float m21;
    float m22;
    float m23;
    float m31;
    float m32;
    float m33;
};

const Matrix33 operator*(const Matrix33& lhs, const Matrix33& rhs);
const Vector3 operator*(const Matrix33& lhs, const Vector3& rhs);

#endif // MATH_MATRIX33_H
