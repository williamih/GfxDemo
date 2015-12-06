#include "Math/Matrix44.h"

#include "Core/Macros.h"

#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix33.h"
#include "Math/Quaternion.h"

Matrix44::Matrix44()
{
    m11 = 1;
    m12 = 0;
    m13 = 0;
    m14 = 0;
    m21 = 0;
    m22 = 1;
    m23 = 0;
    m24 = 0;
    m31 = 0;
    m32 = 0;
    m33 = 1;
    m34 = 0;
    m41 = 0;
    m42 = 0;
    m43 = 0;
    m44 = 1;
}

Matrix44::Matrix44(float a11, float a12, float a13, float a14,
                   float a21, float a22, float a23, float a24,
                   float a31, float a32, float a33, float a34,
                   float a41, float a42, float a43, float a44)
{
    m11 = a11;
    m12 = a12;
    m13 = a13;
    m14 = a14;
    m21 = a21;
    m22 = a22;
    m23 = a23;
    m24 = a24;
    m31 = a31;
    m32 = a32;
    m33 = a33;
    m34 = a34;
    m41 = a41;
    m42 = a42;
    m43 = a43;
    m44 = a44;
}

Matrix44::Matrix44(const Matrix44& other)
{
    operator=(other);
}

Matrix44& Matrix44::operator=(const Matrix44& other)
{
    m11 = other.m11;
    m12 = other.m12;
    m13 = other.m13;
    m14 = other.m14;
    m21 = other.m21;
    m22 = other.m22;
    m23 = other.m23;
    m24 = other.m24;
    m31 = other.m31;
    m32 = other.m32;
    m33 = other.m33;
    m34 = other.m34;
    m41 = other.m41;
    m42 = other.m42;
    m43 = other.m43;
    m44 = other.m44;
    return *this;
}

const Matrix33 Matrix44::UpperLeft3x3() const
{
    return Matrix33(m11, m12, m13,
                    m21, m22, m23,
                    m31, m32, m33);
}

const Matrix44 Matrix44::AffineInverse() const
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

    Matrix44 inv;
    inv.m11 = c11 * recipDet;
    inv.m12 = c21 * recipDet;
    inv.m13 = c31 * recipDet;
    inv.m21 = c12 * recipDet;
    inv.m22 = c22 * recipDet;
    inv.m23 = c32 * recipDet;
    inv.m31 = c13 * recipDet;
    inv.m32 = c23 * recipDet;
    inv.m33 = c33 * recipDet;
    inv.m14 = inv.m11 * -m14 + inv.m12 * -m24 + inv.m13 * -m34;
    inv.m24 = inv.m21 * -m14 + inv.m22 * -m24 + inv.m23 * -m34;
    inv.m34 = inv.m31 * -m14 + inv.m32 * -m24 + inv.m33 * -m34;
    inv.m41 = 0;
    inv.m42 = 0;
    inv.m43 = 0;
    inv.m44 = 1;

    return inv;
}

const Matrix44 Matrix44::Transpose() const
{
    return Matrix44(m11, m21, m31, m41,
                    m12, m22, m32, m42,
                    m13, m23, m33, m43,
                    m14, m24, m34, m44);
}

Matrix44 Matrix44::Rotate(const Quaternion& quat)
{
    float x = quat.x;
    float y = quat.y;
    float z = quat.z;
    float w = quat.w;
    Matrix44 result;
    result.m11 = 1 - 2*y*y - 2*z*z;
    result.m12 = 2*x*y - 2*z*w;
    result.m13 = 2*x*z + 2*y*w;
    result.m21 = 2*x*y + 2*z*w;
    result.m22 = 1 - 2*x*x - 2*z*z;
    result.m23 = 2*y*z - 2*x*w;
    result.m31 = 2*x*z - 2*y*w;
    result.m32 = 2*y*z + 2*x*w;
    result.m33 = 1 - 2*x*x - 2*y*y;
    return result;
}

Matrix44 Matrix44::Translate(const Vector3& vec)
{
    return Matrix44(1.0f, 0.0f, 0.0f, vec.x,
                    0.0f, 1.0f, 0.0f, vec.y,
                    0.0f, 0.0f, 1.0f, vec.z,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix44 Matrix44::Scale(const Vector3& vec)
{
    return Matrix44(vec.x, 0.0f, 0.0f, 0.0f,
                    0.0f, vec.y, 0.0f, 0.0f,
                    0.0f, 0.0f, vec.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

const Matrix44 operator*(const Matrix44& lhs, const Matrix44& rhs)
{
    Matrix44 result;

    result.m11 = lhs.m11 * rhs.m11 + lhs.m12 * rhs.m21 + lhs.m13 * rhs.m31 + lhs.m14 * rhs.m41;
    result.m12 = lhs.m11 * rhs.m12 + lhs.m12 * rhs.m22 + lhs.m13 * rhs.m32 + lhs.m14 * rhs.m42;
    result.m13 = lhs.m11 * rhs.m13 + lhs.m12 * rhs.m23 + lhs.m13 * rhs.m33 + lhs.m14 * rhs.m43;
    result.m14 = lhs.m11 * rhs.m14 + lhs.m12 * rhs.m24 + lhs.m13 * rhs.m34 + lhs.m14 * rhs.m44;

    result.m21 = lhs.m21 * rhs.m11 + lhs.m22 * rhs.m21 + lhs.m23 * rhs.m31 + lhs.m24 * rhs.m41;
    result.m22 = lhs.m21 * rhs.m12 + lhs.m22 * rhs.m22 + lhs.m23 * rhs.m32 + lhs.m24 * rhs.m42;
    result.m23 = lhs.m21 * rhs.m13 + lhs.m22 * rhs.m23 + lhs.m23 * rhs.m33 + lhs.m24 * rhs.m43;
    result.m24 = lhs.m21 * rhs.m14 + lhs.m22 * rhs.m24 + lhs.m23 * rhs.m34 + lhs.m24 * rhs.m44;

    result.m31 = lhs.m31 * rhs.m11 + lhs.m32 * rhs.m21 + lhs.m33 * rhs.m31 + lhs.m34 * rhs.m41;
    result.m32 = lhs.m31 * rhs.m12 + lhs.m32 * rhs.m22 + lhs.m33 * rhs.m32 + lhs.m34 * rhs.m42;
    result.m33 = lhs.m31 * rhs.m13 + lhs.m32 * rhs.m23 + lhs.m33 * rhs.m33 + lhs.m34 * rhs.m43;
    result.m34 = lhs.m31 * rhs.m14 + lhs.m32 * rhs.m24 + lhs.m33 * rhs.m34 + lhs.m34 * rhs.m44;

    result.m41 = lhs.m41 * rhs.m11 + lhs.m42 * rhs.m21 + lhs.m43 * rhs.m31 + lhs.m44 * rhs.m41;
    result.m42 = lhs.m41 * rhs.m12 + lhs.m42 * rhs.m22 + lhs.m43 * rhs.m32 + lhs.m44 * rhs.m42;
    result.m43 = lhs.m41 * rhs.m13 + lhs.m42 * rhs.m23 + lhs.m43 * rhs.m33 + lhs.m44 * rhs.m43;
    result.m44 = lhs.m41 * rhs.m14 + lhs.m42 * rhs.m24 + lhs.m43 * rhs.m34 + lhs.m44 * rhs.m44;

    return result;
}

const Vector4 operator*(const Matrix44& lhs, const Vector4& rhs)
{
    return Vector4(lhs.m11 * rhs.x + lhs.m12 * rhs.y + lhs.m13 * rhs.z + lhs.m14 * rhs.w,
                   lhs.m21 * rhs.x + lhs.m22 * rhs.y + lhs.m23 * rhs.z + lhs.m24 * rhs.w,
                   lhs.m31 * rhs.x + lhs.m32 * rhs.y + lhs.m33 * rhs.z + lhs.m34 * rhs.w,
                   lhs.m41 * rhs.x + lhs.m42 * rhs.y + lhs.m43 * rhs.z + lhs.m44 * rhs.w);
}
