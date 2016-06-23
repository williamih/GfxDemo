/******************************************************************************
 *
 *   GpuShaderPermutations.h
 *
 ***/

/******************************************************************************
 *
 *   This file is private to the GpuDevice module.
 *   Do NOT use this file in client code.
 *
 ***/

#ifndef GPUDEVICE_GPUSHADERPERMUTATIONS_H
#define GPUDEVICE_GPUSHADERPERMUTATIONS_H

#include <vector>
#include "Core/Types.h"
#include "Core/Macros.h"

class GpuDevice;

template<class ShaderProgram>
class GpuShaderPermutations {
public:
    struct Permutation {
        u64 permuteMask;
        u32 next;
        ShaderProgram program;
    };

    GpuShaderPermutations()
        : m_permutations()
        , m_lastAllocated(0)
    {}

    u32 Allocate(u64 permuteMask)
    {
        if (m_permutations.empty()) {
            m_permutations.resize(1);
            m_permutations[0].permuteMask = permuteMask;
            m_permutations[0].next = 0xFFFFFFFFU;
            return 0;
        }

        m_lastAllocated = (m_lastAllocated + 1) % (u32)m_permutations.size();
        u32 pos = m_lastAllocated;
        if (!m_permutations[m_lastAllocated].program.IsInUse())
            return m_lastAllocated;

        for (;;) {
            m_lastAllocated = (m_lastAllocated + 1) % (u32)m_permutations.size();
            if (m_lastAllocated == pos) {
                m_lastAllocated = (u32)m_permutations.size();
                m_permutations.resize(m_permutations.size() + 1);
                m_permutations.back().permuteMask = permuteMask;
                m_permutations.back().next = 0xFFFFFFFFU;
            }
            if (!m_permutations[m_lastAllocated].program.IsInUse())
                break;
        }
        return m_lastAllocated;
    }

    u32 FindPermutationForStates(u32 idxFirstPermutation, u64 stateBitfield)
    {
        for (u32 idx = idxFirstPermutation; idx != 0xFFFFFFFFU; idx = Lookup(idx).next) {
            u64 permuteMask = Lookup(idx).permuteMask;
            if ((stateBitfield & permuteMask) == permuteMask)
                return idx;
        }
        ASSERT(!"A valid shader permutation was not found");
        return 0xFFFFFFFFU;
    }

    void ReleaseChain(u32 idxBegin)
    {
        u32 idxPermutation = idxBegin;
        while (idxPermutation != 0xFFFFFFFFU) {
            Lookup(idxPermutation).program.Release();

            u32 idxTemp = Lookup(idxPermutation).next;
            Lookup(idxPermutation).next = 0xFFFFFFFFU;
            idxPermutation = idxTemp;
        }
    }

    void LoadVertexShader(u32 index, GpuDevice& device, const char* shaderCode, int length)
    {
        Lookup(index).program.LoadVertexShader(device, shaderCode, length);
    }

    void LoadPixelShader(u32 index, GpuDevice& device, const char* shaderCode, int length)
    {
        Lookup(index).program.LoadPixelShader(device, shaderCode, length);
    }

    const Permutation& Lookup(u32 index) const
    {
        return m_permutations[index];
    }

    Permutation& Lookup(u32 index)
    {
        return m_permutations[index];
    }

private:
    std::vector<Permutation> m_permutations;
    u32 m_lastAllocated;
};

#endif // GPUDEVICE_GPUSHADERPERMUTATIONS_H
