#ifndef GPUDEVICE_GPUMATHUTILS_H
#define GPUDEVICE_GPUMATHUTILS_H

class Matrix44;
class Matrix33;
class Vector3;
class Vector4;

namespace GpuMathUtils {
    void FillArrayRowMajor(const Matrix44& m, float array[4][4]);
    void FillArrayColumnMajor(const Matrix44& m, float array[4][4]);
    void FillArrayRowMajor(const Matrix33& m, float array[3][4]);
    void FillArrayColumnMajor(const Matrix33& m, float array[3][4]);

    void FillArray(const Vector4& vec, float array[4]);
    void FillArray(const Vector3& vec, float array[3]);
}

#endif // GPUDEVICE_GPUMATHUTILS_H
