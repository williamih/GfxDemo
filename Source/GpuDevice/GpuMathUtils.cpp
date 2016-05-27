#include "GpuDevice/GpuMathUtils.h"

#include "Math/Matrix44.h"
#include "Math/Matrix33.h"
#include "Math/Vector4.h"
#include "Math/Vector3.h"

void GpuMathUtils::FillArrayRowMajor(const Matrix44& m, float array[4][4])
{
    array[0][0] = m.m11;
    array[0][1] = m.m12;
    array[0][2] = m.m13;
    array[0][3] = m.m14;

    array[1][0] = m.m21;
    array[1][1] = m.m22;
    array[1][2] = m.m23;
    array[1][3] = m.m24;

    array[2][0] = m.m31;
    array[2][1] = m.m32;
    array[2][2] = m.m33;
    array[2][3] = m.m34;

    array[3][0] = m.m41;
    array[3][1] = m.m42;
    array[3][2] = m.m43;
    array[3][3] = m.m44;
}

void GpuMathUtils::FillArrayColumnMajor(const Matrix44& m, float array[4][4])
{
    array[0][0] = m.m11;
    array[0][1] = m.m21;
    array[0][2] = m.m31;
    array[0][3] = m.m41;

    array[1][0] = m.m12;
    array[1][1] = m.m22;
    array[1][2] = m.m32;
    array[1][3] = m.m42;

    array[2][0] = m.m13;
    array[2][1] = m.m23;
    array[2][2] = m.m33;
    array[2][3] = m.m43;

    array[3][0] = m.m14;
    array[3][1] = m.m24;
    array[3][2] = m.m34;
    array[3][3] = m.m44;
}

void GpuMathUtils::FillArrayRowMajor(const Matrix33& m, float array[3][4])
{
    array[0][0] = m.m11;
    array[0][1] = m.m12;
    array[0][2] = m.m13;
    array[0][3] = 0.0f; // placeholder for alignment purposes

    array[1][0] = m.m21;
    array[1][1] = m.m22;
    array[1][2] = m.m23;
    array[1][3] = 0.0f; // placeholder for alignment purposes

    array[2][0] = m.m31;
    array[2][1] = m.m32;
    array[2][2] = m.m33;
    array[2][3] = 0.0f; // placeholder for alignment purposes
}

void GpuMathUtils::FillArrayColumnMajor(const Matrix33& m, float array[3][4])
{
    array[0][0] = m.m11;
    array[0][1] = m.m21;
    array[0][2] = m.m31;
    array[0][3] = 0.0f; // placeholder for alignment purposes

    array[1][0] = m.m12;
    array[1][1] = m.m22;
    array[1][2] = m.m32;
    array[1][3] = 0.0f; // placeholder for alignment purposes

    array[2][0] = m.m13;
    array[2][1] = m.m23;
    array[2][2] = m.m33;
    array[2][3] = 0.0f; // placeholder for alignment purposes
}

void GpuMathUtils::FillArray(const Vector4& vec, float array[4])
{
    array[0] = vec.x;
    array[1] = vec.y;
    array[2] = vec.z;
    array[3] = vec.w;
}

void GpuMathUtils::FillArray(const Vector3& vec, float array[3])
{
    array[0] = vec.x;
    array[1] = vec.y;
    array[2] = vec.z;
}
