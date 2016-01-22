#include "GpuDevice/GpuShaderLoad.h"

#include "Core/Macros.h"
#include "Core/Endian.h"

#define FOURCC(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

namespace GpuShaderLoad {
    struct Header {
        u32 fourCC;
        u32 version;
        u32 languageFourCC;
        u32 nPermutations;

        void FixEndian()
        {
            fourCC = EndianSwapLE32(fourCC);
            version = EndianSwapLE32(version);
            languageFourCC = EndianSwapLE32(languageFourCC);
            nPermutations = EndianSwapLE32(nPermutations);
        }
    };

    struct PermutationHeader {
        u64 permuteMask;
        u32 vsLength;
        u32 psLength;
        u32 ofsNextPermutation; // offset to next permutation
        u32 _pad;

        void FixEndian()
        {
            permuteMask = EndianSwapLE64(permuteMask);
            vsLength = EndianSwapLE32(vsLength);
            psLength = EndianSwapLE32(psLength);
            ofsNextPermutation = EndianSwapLE32(ofsNextPermutation);
        }
    };
}

void GpuShaderLoad::ReadShaderFile(const char* data,
                                   int length,
                                   u32 targetLanguageFourCC,
                                   GpuShaderLoad::ReadCallback callback,
                                   void* userdata)
{
    if (length < sizeof(GpuShaderLoad::Header))
        FATAL("Shader file doesn't contain a full header");

    Header header;
    memcpy(&header, data, sizeof header);
    header.FixEndian();
    if (header.fourCC != FOURCC('S', 'H', 'D', 'R'))
        FATAL("Shader file has incorrect FourCC");
    if (header.languageFourCC != targetLanguageFourCC)
        FATAL("Shader file is for another shader language than the one requested");

    const u8* ptr = (const u8*)(data + sizeof(Header));
    for (u32 i = 0; i < header.nPermutations; ++i) {
        PermutationHeader permuteHeader;
        memcpy(&permuteHeader, ptr, sizeof permuteHeader);
        permuteHeader.FixEndian();
        const char* shaderCode = (const char*)(ptr + sizeof(PermutationHeader));

        callback(
            READACTION_NEW_PERMUTATION,
            permuteHeader.permuteMask,
            NULL,
            0,
            userdata
        );

        callback(
            READACTION_PROVIDE_VS_CODE,
            permuteHeader.permuteMask,
            shaderCode,
            permuteHeader.vsLength,
            userdata
        );
        shaderCode += permuteHeader.vsLength;
        callback(
            READACTION_PROVIDE_PS_CODE,
            permuteHeader.permuteMask,
            shaderCode,
            permuteHeader.psLength,
            userdata
        );

        ptr += permuteHeader.ofsNextPermutation;
    }
}
