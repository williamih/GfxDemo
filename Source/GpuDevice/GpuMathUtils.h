#ifndef GPUDEVICE_GPUMATHUTILS_H
#define GPUDEVICE_GPUMATHUTILS_H

class Matrix44;
class Matrix33;

namespace GpuMathUtils {
    void FillArrayRowMajor(const Matrix44& m, float array[4][4]);
    void FillArrayColumnMajor(const Matrix44& m, float array[4][4]);
    void FillArrayRowMajor(const Matrix33& m, float array[3][4]);
    void FillArrayColumnMajor(const Matrix33& m, float array[3][4]);
}

#endif // GPUDEVICE_GPUMATHUTILS_H
