/******************************************************************************
 *
 *   GpuShaderLoad.h
 *
 ***/

/******************************************************************************
 *
 *   This file is private to the GpuDevice module.
 *   Do NOT use this file in client code.
 *
 ***/

#ifndef GPUDEVICE_GPUSHADERLOAD_H
#define GPUDEVICE_GPUSHADERLOAD_H

#include "GpuDevice/GpuDevice.h"
#include "GpuDevice/GpuShaderPermutations.h"

namespace GpuShaderLoad {
    enum ReadAction {
        READACTION_NEW_PERMUTATION,
        READACTION_PROVIDE_VS_CODE,
        READACTION_PROVIDE_PS_CODE,
    };

    typedef void (*ReadCallback)(ReadAction action,
                                 u64 permuteMask,
                                 const char* shaderCode,
                                 int length,
                                 void* userdata);

    void ReadShaderFile(const char* data,
                        int length,
                        u32 targetLanguageFourCC,
                        ReadCallback callback,
                        void* userdata);

    template<class ShaderProgram>
    struct ShaderReadContext {
        GpuDevice* device;
        GpuShaderPermutations<ShaderProgram>* permutations;
        u32 idxHead;
        u32 idxTail;
    };

    template<class ShaderProgram>
    void LoadShaderCallback(ReadAction action,
                            u64 permuteMask,
                            const char* shaderCode,
                            int length,
                            void* userdata)
    {
        ShaderReadContext<ShaderProgram>* ctx = (ShaderReadContext<ShaderProgram>*)userdata;
        GpuShaderPermutations<ShaderProgram>* permutations = ctx->permutations;
        GpuDevice* device = ctx->device;

        switch (action) {
            case READACTION_NEW_PERMUTATION: {
                u32 idxPermutation = permutations->Allocate(permuteMask);
                if (ctx->idxHead == 0xFFFFFFFF)
                    ctx->idxHead = idxPermutation;
                if (ctx->idxTail != 0xFFFFFFFF)
                    permutations->Lookup(ctx->idxTail).next = idxPermutation;
                ctx->idxTail = idxPermutation;
                break;
            }
            case READACTION_PROVIDE_VS_CODE:
                permutations->LoadVertexShader(ctx->idxTail, device, shaderCode, length);
                break;
            case READACTION_PROVIDE_PS_CODE:
                permutations->LoadPixelShader(ctx->idxTail, device, shaderCode, length);
                break;
        }
    }

    template<class ShaderProgram>
    u32 LoadShader(const char* data, int length, u32 targetLanguageFourCC,
                   GpuDevice* device,
                   GpuShaderPermutations<ShaderProgram>& permutations)
    {
        ShaderReadContext<ShaderProgram> ctx;
        ctx.device = device;
        ctx.permutations = &permutations;
        ctx.idxHead = 0xFFFFFFFF;
        ctx.idxTail = 0xFFFFFFFF;

        ReadShaderFile(
            data,
            length,
            targetLanguageFourCC,
            &LoadShaderCallback<ShaderProgram>,
            (void*)&ctx
        );
        return ctx.idxHead;
    }
}

#endif // GPUDEVICE_GPUSHADERLOAD_H
