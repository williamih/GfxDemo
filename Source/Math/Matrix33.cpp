#include "Math/Matrix33.h"
#include <math.h>
#include "Core/Macros.h"

Matrix33::Matrix33()
{
    m11 = 1;
    m12 = 0;
    m13 = 0;
    m21 = 0;
    m22 = 1;
    m23 = 0;
    m31 = 0;
    m32 = 0;
    m33 = 1;
}

Matrix33::Matrix33(float a11, float a12, float a13,
                   float a21, float a22, float a23,
                   float a31, float a32, float a33)
{
    m11 = a11;
    m12 = a12;
    m13 = a13;
    m21 = a21;
    m22 = a22;
    m23 = a23;
    m31 = a31;
    m32 = a32;
    m33 = a33;
}

Matrix33::Matrix33(const Matrix33& other)
{
    operator=(other);
}

Matrix33& Matrix33::operator=(const Matrix33& other)
{
    m11 = other.m11;
    m12 = other.m12;
    m13 = other.m13;
    m21 = other.m21;
    m22 = other.m22;
    m23 = other.m23;
    m31 = other.m31;
    m32 = other.m32;
    m33 = other.m33;
    return *this;
}

Matrix33 Matrix33::FromEulerAnglesZXY(float z, float x, float y)
{
    float cosZ = cosf(z);
    float sinZ = sinf(z);
    float cosX = cosf(x);
    float sinX = sinf(x);
    float cosY = cosf(y);
    float sinY = sinf(y);
    Matrix33 rotateZ(cosZ, -sinZ, 0.0f,
                     sinZ, cosZ, 0.0f,
                     0.0f, 0.0f, 1.0f);
    Matrix33 rotateX(1.0f, 0.0f, 0.0f,
                     0.0f, cosX, -sinX,
                     0.0f, sinX, cosX);
    Matrix33 rotateY(cosY, 0.0f, sinY,
                     0.0f, 1.0f, 0.0f,
                     -sinY, 0.0f, cosY);
    return rotateZ * rotateX * rotateY;
}

const Matrix33 Matrix33::Inverse() const
{
    float c11 = +(m22 * m33 - m23 * m32);
    float c12 = -(m21 * m33 - m23 * m31);
    float c13 = +(m21 * m32 - m22 * m31);
    float c21 = -(m12 * m33 - m13 * m32);
    float c22 = +(m11 * m33 - m13 * m31);
    float c23 = -(m11 * m32 - m12 * m31);
    float c31 = +(m12 * m23 - m13 * m22);
    float c32 = -(m11 * m23 - m13 * m21);
    float c33 = +(m11 * m22 - m12 * m21);

    float det = m11 * c11 + m12 * c12 + m13 * c13;
    ASSERT(det != 0);
    float recipDet = 1 / det;

    Matrix33 inv;
    inv.m11 = c11 * recipDet;
    inv.m12 = c21 * recipDet;
    inv.m13 = c31 * recipDet;
    inv.m21 = c12 * recipDet;
    inv.m22 = c22 * recipDet;
    inv.m23 = c32 * recipDet;
    inv.m31 = c13 * recipDet;
    inv.m32 = c23 * recipDet;
    inv.m33 = c33 * recipDet;

    return inv;
}

const Matrix33 Matrix33::Transpose() const
{
    return Matrix33(m11, m21, m31,
                    m12, m22, m32,
                    m13, m23, m33);
}

const Matrix33 operator*(const Matrix33& lhs, const Matrix33& rhs)
{
    Matrix33 result;

    result.m11 = lhs.m11 * rhs.m11 + lhs.m12 * rhs.m21 + lhs.m13 * rhs.m31;
    result.m12 = lhs.m11 * rhs.m12 + lhs.m12 * rhs.m22 + lhs.m13 * rhs.m32;
    result.m13 = lhs.m11 * rhs.m13 + lhs.m12 * rhs.m23 + lhs.m13 * rhs.m33;

    result.m21 = lhs.m21 * rhs.m11 + lhs.m22 * rhs.m21 + lhs.m23 * rhs.m31;
    result.m22 = lhs.m21 * rhs.m12 + lhs.m22 * rhs.m22 + lhs.m23 * rhs.m32;
    result.m23 = lhs.m21 * rhs.m13 + lhs.m22 * rhs.m23 + lhs.m23 * rhs.m33;

    result.m31 = lhs.m31 * rhs.m11 + lhs.m32 * rhs.m21 + lhs.m33 * rhs.m31;
    result.m32 = lhs.m31 * rhs.m12 + lhs.m32 * rhs.m22 + lhs.m33 * rhs.m32;
    result.m33 = lhs.m31 * rhs.m13 + lhs.m32 * rhs.m23 + lhs.m33 * rhs.m33;

    return result;
}

const Vector3 operator*(const Matrix33& lhs, const Vector3& rhs)
{
    return Vector3(lhs.m11 * rhs.x + lhs.m12 * rhs.y + lhs.m13 * rhs.z,
                    lhs.m21 * rhs.x + lhs.m22 * rhs.y + lhs.m23 * rhs.z,
                    lhs.m31 * rhs.x + lhs.m32 * rhs.y + lhs.m33 * rhs.z);
}
