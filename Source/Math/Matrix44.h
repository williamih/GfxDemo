#ifndef MATH_MATRIX44_H
#define MATH_MATRIX44_H

class Vector3;
class Vector4;
class Matrix33;
class Quaternion;

class Matrix44 {
public:
    Matrix44();

    Matrix44(float a11, float a12, float a13, float a14,
             float a21, float a22, float a23, float a24,
             float a31, float a32, float a33, float a34,
             float a41, float a42, float a43, float a44);

    Matrix44(const Matrix44& other);

    Matrix44& operator=(const Matrix44& other);

    const Matrix33 UpperLeft3x3() const;
    const Matrix44 AffineInverse() const;
    const Matrix44 Transpose() const;

    static Matrix44 Rotate(const Quaternion& quat);
    static Matrix44 Translate(const Vector3& vec);
    static Matrix44 Scale(const Vector3& vec);

    float m11;
    float m12;
    float m13;
    float m14;
    float m21;
    float m22;
    float m23;
    float m24;
    float m31;
    float m32;
    float m33;
    float m34;
    float m41;
    float m42;
    float m43;
    float m44;
};

const Matrix44 operator*(const Matrix44& lhs, const Matrix44& rhs);
const Vector4 operator*(const Matrix44& lhs, const Vector4& rhs);

#endif // MATH_MATRIX44_H
