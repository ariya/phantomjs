/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "VectorMath.h"

#if OS(DARWIN)
#include <Accelerate/Accelerate.h>
#endif

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#if HAVE(ARM_NEON_INTRINSICS)
#include <arm_neon.h>
#endif

#include <algorithm>
#include <math.h>

namespace WebCore {

namespace VectorMath {

#if OS(DARWIN)
// On the Mac we use the highly optimized versions in Accelerate.framework
// In 32-bit mode (__ppc__ or __i386__) <Accelerate/Accelerate.h> includes <vecLib/vDSP_translate.h> which defines macros of the same name as
// our namespaced function names, so we must handle this case differently. Other architectures (64bit, ARM, etc.) do not include this header file.

void vsmul(const float* sourceP, int sourceStride, const float* scale, float* destP, int destStride, size_t framesToProcess)
{
#if defined(__ppc__) || defined(__i386__)
    ::vsmul(sourceP, sourceStride, scale, destP, destStride, framesToProcess);
#else
    vDSP_vsmul(sourceP, sourceStride, scale, destP, destStride, framesToProcess);
#endif
}

void vadd(const float* source1P, int sourceStride1, const float* source2P, int sourceStride2, float* destP, int destStride, size_t framesToProcess)
{
#if defined(__ppc__) || defined(__i386__)
    ::vadd(source1P, sourceStride1, source2P, sourceStride2, destP, destStride, framesToProcess);
#else
    vDSP_vadd(source1P, sourceStride1, source2P, sourceStride2, destP, destStride, framesToProcess);
#endif
}

void vmul(const float* source1P, int sourceStride1, const float* source2P, int sourceStride2, float* destP, int destStride, size_t framesToProcess)
{
#if defined(__ppc__) || defined(__i386__)
    ::vmul(source1P, sourceStride1, source2P, sourceStride2, destP, destStride, framesToProcess);
#else
    vDSP_vmul(source1P, sourceStride1, source2P, sourceStride2, destP, destStride, framesToProcess);
#endif
}

void zvmul(const float* real1P, const float* imag1P, const float* real2P, const float* imag2P, float* realDestP, float* imagDestP, size_t framesToProcess)
{
    DSPSplitComplex sc1;
    DSPSplitComplex sc2;
    DSPSplitComplex dest;
    sc1.realp = const_cast<float*>(real1P);
    sc1.imagp = const_cast<float*>(imag1P);
    sc2.realp = const_cast<float*>(real2P);
    sc2.imagp = const_cast<float*>(imag2P);
    dest.realp = realDestP;
    dest.imagp = imagDestP;
#if defined(__ppc__) || defined(__i386__)
    ::zvmul(&sc1, 1, &sc2, 1, &dest, 1, framesToProcess, 1);
#else
    vDSP_zvmul(&sc1, 1, &sc2, 1, &dest, 1, framesToProcess, 1);
#endif
}

void vsma(const float* sourceP, int sourceStride, const float* scale, float* destP, int destStride, size_t framesToProcess)
{
    vDSP_vsma(sourceP, sourceStride, scale, destP, destStride, destP, destStride, framesToProcess);
}

void vmaxmgv(const float* sourceP, int sourceStride, float* maxP, size_t framesToProcess)
{
    vDSP_maxmgv(sourceP, sourceStride, maxP, framesToProcess);
}

void vsvesq(const float* sourceP, int sourceStride, float* sumP, size_t framesToProcess)
{
    vDSP_svesq(const_cast<float*>(sourceP), sourceStride, sumP, framesToProcess);
}

void vclip(const float* sourceP, int sourceStride, const float* lowThresholdP, const float* highThresholdP, float* destP, int destStride, size_t framesToProcess)
{
    vDSP_vclip(const_cast<float*>(sourceP), sourceStride, const_cast<float*>(lowThresholdP), const_cast<float*>(highThresholdP), destP, destStride, framesToProcess);
}
#else

void vsma(const float* sourceP, int sourceStride, const float* scale, float* destP, int destStride, size_t framesToProcess)
{
    int n = framesToProcess;

#ifdef __SSE2__
    if ((sourceStride == 1) && (destStride == 1)) {
        float k = *scale;

        // If the sourceP address is not 16-byte aligned, the first several frames (at most three) should be processed separately.
        while ((reinterpret_cast<uintptr_t>(sourceP) & 0x0F) && n) {
            *destP += k * *sourceP;
            sourceP++;
            destP++;
            n--;
        }

        // Now the sourceP is aligned, use SSE.
        int tailFrames = n % 4;
        const float* endP = destP + n - tailFrames;

        __m128 pSource;
        __m128 dest;
        __m128 temp;
        __m128 mScale = _mm_set_ps1(k);

        bool destAligned = !(reinterpret_cast<uintptr_t>(destP) & 0x0F);

#define SSE2_MULT_ADD(loadInstr, storeInstr)        \
            while (destP < endP)                    \
            {                                       \
                pSource = _mm_load_ps(sourceP);     \
                temp = _mm_mul_ps(pSource, mScale); \
                dest = _mm_##loadInstr##_ps(destP); \
                dest = _mm_add_ps(dest, temp);      \
                _mm_##storeInstr##_ps(destP, dest); \
                sourceP += 4;                       \
                destP += 4;                         \
            }

        if (destAligned) 
            SSE2_MULT_ADD(load, store)
        else 
            SSE2_MULT_ADD(loadu, storeu)

        n = tailFrames;
    }
#elif HAVE(ARM_NEON_INTRINSICS)
    if ((sourceStride == 1) && (destStride == 1)) {
        int tailFrames = n % 4;
        const float* endP = destP + n - tailFrames;

        float32x4_t k = vdupq_n_f32(*scale);
        while (destP < endP) {
            float32x4_t source = vld1q_f32(sourceP);
            float32x4_t dest = vld1q_f32(destP);

            dest = vmlaq_f32(dest, source, k);
            vst1q_f32(destP, dest);

            sourceP += 4;
            destP += 4;
        }
        n = tailFrames;
    }
#endif
    while (n) {
        *destP += *sourceP * *scale;
        sourceP += sourceStride;
        destP += destStride;
        n--;
    }
}

void vsmul(const float* sourceP, int sourceStride, const float* scale, float* destP, int destStride, size_t framesToProcess)
{
    int n = framesToProcess;

#ifdef __SSE2__
    if ((sourceStride == 1) && (destStride == 1)) {
        float k = *scale;

        // If the sourceP address is not 16-byte aligned, the first several frames (at most three) should be processed separately.
        while ((reinterpret_cast<size_t>(sourceP) & 0x0F) && n) {
            *destP = k * *sourceP;
            sourceP++;
            destP++;
            n--;
        }

        // Now the sourceP address is aligned and start to apply SSE.
        int group = n / 4;
        __m128 mScale = _mm_set_ps1(k);
        __m128* pSource;
        __m128* pDest;
        __m128 dest;


        if (reinterpret_cast<size_t>(destP) & 0x0F) {
            while (group--) {
                pSource = reinterpret_cast<__m128*>(const_cast<float*>(sourceP));
                dest = _mm_mul_ps(*pSource, mScale);
                _mm_storeu_ps(destP, dest);

                sourceP += 4;
                destP += 4;
            }
        } else {
            while (group--) {
                pSource = reinterpret_cast<__m128*>(const_cast<float*>(sourceP));
                pDest = reinterpret_cast<__m128*>(destP);
                *pDest = _mm_mul_ps(*pSource, mScale);

                sourceP += 4;
                destP += 4;
            }
        }

        // Non-SSE handling for remaining frames which is less than 4.
        n %= 4;
        while (n) {
            *destP = k * *sourceP;
            sourceP++;
            destP++;
            n--;
        }
    } else { // If strides are not 1, rollback to normal algorithm.
#elif HAVE(ARM_NEON_INTRINSICS)
    if ((sourceStride == 1) && (destStride == 1)) {
        float k = *scale;
        int tailFrames = n % 4;
        const float* endP = destP + n - tailFrames;

        while (destP < endP) {
            float32x4_t source = vld1q_f32(sourceP);
            vst1q_f32(destP, vmulq_n_f32(source, k));

            sourceP += 4;
            destP += 4;
        }
        n = tailFrames;
    }
#endif
    float k = *scale;
    while (n--) {
        *destP = k * *sourceP;
        sourceP += sourceStride;
        destP += destStride;
    }
#ifdef __SSE2__
    }
#endif
}

void vadd(const float* source1P, int sourceStride1, const float* source2P, int sourceStride2, float* destP, int destStride, size_t framesToProcess)
{
    int n = framesToProcess;

#ifdef __SSE2__
    if ((sourceStride1 ==1) && (sourceStride2 == 1) && (destStride == 1)) {
        // If the sourceP address is not 16-byte aligned, the first several frames (at most three) should be processed separately.
        while ((reinterpret_cast<size_t>(source1P) & 0x0F) && n) {
            *destP = *source1P + *source2P;
            source1P++;
            source2P++;
            destP++;
            n--;
        }

        // Now the source1P address is aligned and start to apply SSE.
        int group = n / 4;
        __m128* pSource1;
        __m128* pSource2;
        __m128* pDest;
        __m128 source2;
        __m128 dest;

        bool source2Aligned = !(reinterpret_cast<size_t>(source2P) & 0x0F);
        bool destAligned = !(reinterpret_cast<size_t>(destP) & 0x0F);

        if (source2Aligned && destAligned) { // all aligned
            while (group--) {
                pSource1 = reinterpret_cast<__m128*>(const_cast<float*>(source1P));
                pSource2 = reinterpret_cast<__m128*>(const_cast<float*>(source2P));
                pDest = reinterpret_cast<__m128*>(destP);
                *pDest = _mm_add_ps(*pSource1, *pSource2);

                source1P += 4;
                source2P += 4;
                destP += 4;
            }

        } else if (source2Aligned && !destAligned) { // source2 aligned but dest not aligned 
            while (group--) {
                pSource1 = reinterpret_cast<__m128*>(const_cast<float*>(source1P));
                pSource2 = reinterpret_cast<__m128*>(const_cast<float*>(source2P));
                dest = _mm_add_ps(*pSource1, *pSource2);
                _mm_storeu_ps(destP, dest);

                source1P += 4;
                source2P += 4;
                destP += 4;
            }

        } else if (!source2Aligned && destAligned) { // source2 not aligned but dest aligned 
            while (group--) {
                pSource1 = reinterpret_cast<__m128*>(const_cast<float*>(source1P));
                source2 = _mm_loadu_ps(source2P);
                pDest = reinterpret_cast<__m128*>(destP);
                *pDest = _mm_add_ps(*pSource1, source2);

                source1P += 4;
                source2P += 4;
                destP += 4;
            }
        } else if (!source2Aligned && !destAligned) { // both source2 and dest not aligned 
            while (group--) {
                pSource1 = reinterpret_cast<__m128*>(const_cast<float*>(source1P));
                source2 = _mm_loadu_ps(source2P);
                dest = _mm_add_ps(*pSource1, source2);
                _mm_storeu_ps(destP, dest);

                source1P += 4;
                source2P += 4;
                destP += 4;
            }
        }

        // Non-SSE handling for remaining frames which is less than 4.
        n %= 4;
        while (n) {
            *destP = *source1P + *source2P;
            source1P++;
            source2P++;
            destP++;
            n--;
        }
    } else { // if strides are not 1, rollback to normal algorithm
#elif HAVE(ARM_NEON_INTRINSICS)
    if ((sourceStride1 ==1) && (sourceStride2 == 1) && (destStride == 1)) {
        int tailFrames = n % 4;
        const float* endP = destP + n - tailFrames;

        while (destP < endP) {
            float32x4_t source1 = vld1q_f32(source1P);
            float32x4_t source2 = vld1q_f32(source2P);
            vst1q_f32(destP, vaddq_f32(source1, source2));

            source1P += 4;
            source2P += 4;
            destP += 4;
        }
        n = tailFrames;
    }
#endif
    while (n--) {
        *destP = *source1P + *source2P;
        source1P += sourceStride1;
        source2P += sourceStride2;
        destP += destStride;
    }
#ifdef __SSE2__
    }
#endif
}

void vmul(const float* source1P, int sourceStride1, const float* source2P, int sourceStride2, float* destP, int destStride, size_t framesToProcess)
{

    int n = framesToProcess;

#ifdef __SSE2__
    if ((sourceStride1 == 1) && (sourceStride2 == 1) && (destStride == 1)) {
        // If the source1P address is not 16-byte aligned, the first several frames (at most three) should be processed separately.
        while ((reinterpret_cast<uintptr_t>(source1P) & 0x0F) && n) {
            *destP = *source1P * *source2P;
            source1P++;
            source2P++;
            destP++;
            n--;
        }

        // Now the source1P address aligned and start to apply SSE.
        int tailFrames = n % 4;
        const float* endP = destP + n - tailFrames;
        __m128 pSource1;
        __m128 pSource2;
        __m128 dest;

        bool source2Aligned = !(reinterpret_cast<uintptr_t>(source2P) & 0x0F);
        bool destAligned = !(reinterpret_cast<uintptr_t>(destP) & 0x0F);

#define SSE2_MULT(loadInstr, storeInstr)                   \
            while (destP < endP)                           \
            {                                              \
                pSource1 = _mm_load_ps(source1P);          \
                pSource2 = _mm_##loadInstr##_ps(source2P); \
                dest = _mm_mul_ps(pSource1, pSource2);     \
                _mm_##storeInstr##_ps(destP, dest);        \
                source1P += 4;                             \
                source2P += 4;                             \
                destP += 4;                                \
            }

        if (source2Aligned && destAligned) // Both aligned.
            SSE2_MULT(load, store)
        else if (source2Aligned && !destAligned) // Source2 is aligned but dest not.
            SSE2_MULT(load, storeu)
        else if (!source2Aligned && destAligned) // Dest is aligned but source2 not.
            SSE2_MULT(loadu, store)
        else // Neither aligned.
            SSE2_MULT(loadu, storeu)

        n = tailFrames;
    }
#elif HAVE(ARM_NEON_INTRINSICS)
    if ((sourceStride1 ==1) && (sourceStride2 == 1) && (destStride == 1)) {
        int tailFrames = n % 4;
        const float* endP = destP + n - tailFrames;

        while (destP < endP) {
            float32x4_t source1 = vld1q_f32(source1P);
            float32x4_t source2 = vld1q_f32(source2P);
            vst1q_f32(destP, vmulq_f32(source1, source2));

            source1P += 4;
            source2P += 4;
            destP += 4;
        }
        n = tailFrames;
    }
#endif
    while (n) {
        *destP = *source1P * *source2P;
        source1P += sourceStride1;
        source2P += sourceStride2;
        destP += destStride;
        n--;
    }
}

void zvmul(const float* real1P, const float* imag1P, const float* real2P, const float* imag2P, float* realDestP, float* imagDestP, size_t framesToProcess)
{
    unsigned i = 0;
#ifdef __SSE2__
    // Only use the SSE optimization in the very common case that all addresses are 16-byte aligned. 
    // Otherwise, fall through to the scalar code below.
    if (!(reinterpret_cast<uintptr_t>(real1P) & 0x0F)
        && !(reinterpret_cast<uintptr_t>(imag1P) & 0x0F)
        && !(reinterpret_cast<uintptr_t>(real2P) & 0x0F)
        && !(reinterpret_cast<uintptr_t>(imag2P) & 0x0F)
        && !(reinterpret_cast<uintptr_t>(realDestP) & 0x0F)
        && !(reinterpret_cast<uintptr_t>(imagDestP) & 0x0F)) {
        
        unsigned endSize = framesToProcess - framesToProcess % 4;
        while (i < endSize) {
            __m128 real1 = _mm_load_ps(real1P + i);
            __m128 real2 = _mm_load_ps(real2P + i);
            __m128 imag1 = _mm_load_ps(imag1P + i);
            __m128 imag2 = _mm_load_ps(imag2P + i);
            __m128 real = _mm_mul_ps(real1, real2);
            real = _mm_sub_ps(real, _mm_mul_ps(imag1, imag2));
            __m128 imag = _mm_mul_ps(real1, imag2);
            imag = _mm_add_ps(imag, _mm_mul_ps(imag1, real2));
            _mm_store_ps(realDestP + i, real);
            _mm_store_ps(imagDestP + i, imag);
            i += 4;
        }
    }
#elif HAVE(ARM_NEON_INTRINSICS)
        unsigned endSize = framesToProcess - framesToProcess % 4;
        while (i < endSize) {
            float32x4_t real1 = vld1q_f32(real1P + i);
            float32x4_t real2 = vld1q_f32(real2P + i);
            float32x4_t imag1 = vld1q_f32(imag1P + i);
            float32x4_t imag2 = vld1q_f32(imag2P + i);

            float32x4_t realResult = vmlsq_f32(vmulq_f32(real1, real2), imag1, imag2);
            float32x4_t imagResult = vmlaq_f32(vmulq_f32(real1, imag2), imag1, real2);

            vst1q_f32(realDestP + i, realResult);
            vst1q_f32(imagDestP + i, imagResult);

            i += 4;
        }
#endif
    for (; i < framesToProcess; ++i) {
        // Read and compute result before storing them, in case the
        // destination is the same as one of the sources.
        float realResult = real1P[i] * real2P[i] - imag1P[i] * imag2P[i];
        float imagResult = real1P[i] * imag2P[i] + imag1P[i] * real2P[i];

        realDestP[i] = realResult;
        imagDestP[i] = imagResult;
    }
}

void vsvesq(const float* sourceP, int sourceStride, float* sumP, size_t framesToProcess)
{
    int n = framesToProcess;
    float sum = 0;

#ifdef __SSE2__ 
    if (sourceStride == 1) { 
        // If the sourceP address is not 16-byte aligned, the first several frames (at most three) should be processed separately. 
        while ((reinterpret_cast<uintptr_t>(sourceP) & 0x0F) && n) { 
            float sample = *sourceP; 
            sum += sample * sample; 
            sourceP++; 
            n--; 
        } 
 
        // Now the sourceP is aligned, use SSE.
        int tailFrames = n % 4; 
        const float* endP = sourceP + n - tailFrames; 
        __m128 source; 
        __m128 mSum = _mm_setzero_ps(); 
 
        while (sourceP < endP) { 
            source = _mm_load_ps(sourceP); 
            source = _mm_mul_ps(source, source); 
            mSum = _mm_add_ps(mSum, source); 
            sourceP += 4; 
        } 
 
        // Summarize the SSE results. 
        const float* groupSumP = reinterpret_cast<float*>(&mSum); 
        sum += groupSumP[0] + groupSumP[1] + groupSumP[2] + groupSumP[3]; 
 
        n = tailFrames; 
    } 
#elif HAVE(ARM_NEON_INTRINSICS)
    if (sourceStride == 1) {
        int tailFrames = n % 4;
        const float* endP = sourceP + n - tailFrames;

        float32x4_t fourSum = vdupq_n_f32(0);
        while (sourceP < endP) {
            float32x4_t source = vld1q_f32(sourceP);
            fourSum = vmlaq_f32(fourSum, source, source);
            sourceP += 4;
        }
        float32x2_t twoSum = vadd_f32(vget_low_f32(fourSum), vget_high_f32(fourSum));

        float groupSum[2];
        vst1_f32(groupSum, twoSum);
        sum += groupSum[0] + groupSum[1];

        n = tailFrames;
    }
#endif

    while (n--) {
        float sample = *sourceP;
        sum += sample * sample;
        sourceP += sourceStride;
    }

    ASSERT(sumP);
    *sumP = sum;
}

void vmaxmgv(const float* sourceP, int sourceStride, float* maxP, size_t framesToProcess)
{
    int n = framesToProcess;
    float max = 0;

#ifdef __SSE2__
    if (sourceStride == 1) {
        // If the sourceP address is not 16-byte aligned, the first several frames (at most three) should be processed separately.
        while ((reinterpret_cast<uintptr_t>(sourceP) & 0x0F) && n) {
            max = std::max(max, fabsf(*sourceP));
            sourceP++;
            n--;
        }

        // Now the sourceP is aligned, use SSE.
        int tailFrames = n % 4;
        const float* endP = sourceP + n - tailFrames;
        __m128 source;
        __m128 mMax = _mm_setzero_ps();
        int mask = 0x7FFFFFFF;
        __m128 mMask = _mm_set1_ps(*reinterpret_cast<float*>(&mask));

        while (sourceP < endP) {
            source = _mm_load_ps(sourceP);
            // Calculate the absolute value by anding source with mask, the sign bit is set to 0.
            source = _mm_and_ps(source, mMask);
            mMax = _mm_max_ps(mMax, source);
            sourceP += 4;
        }

        // Get max from the SSE results.
        const float* groupMaxP = reinterpret_cast<float*>(&mMax);
        max = std::max(max, groupMaxP[0]);
        max = std::max(max, groupMaxP[1]);
        max = std::max(max, groupMaxP[2]);
        max = std::max(max, groupMaxP[3]);

        n = tailFrames;
    }
#elif HAVE(ARM_NEON_INTRINSICS)
    if (sourceStride == 1) {
        int tailFrames = n % 4;
        const float* endP = sourceP + n - tailFrames;

        float32x4_t fourMax = vdupq_n_f32(0);
        while (sourceP < endP) {
            float32x4_t source = vld1q_f32(sourceP);
            fourMax = vmaxq_f32(fourMax, vabsq_f32(source));
            sourceP += 4;
        }
        float32x2_t twoMax = vmax_f32(vget_low_f32(fourMax), vget_high_f32(fourMax));

        float groupMax[2];
        vst1_f32(groupMax, twoMax);
        max = std::max(groupMax[0], groupMax[1]);

        n = tailFrames;
    }
#endif

    while (n--) {
        max = std::max(max, fabsf(*sourceP));
        sourceP += sourceStride;
    }

    ASSERT(maxP);
    *maxP = max;
}

void vclip(const float* sourceP, int sourceStride, const float* lowThresholdP, const float* highThresholdP, float* destP, int destStride, size_t framesToProcess)
{
    int n = framesToProcess;
    float lowThreshold = *lowThresholdP;
    float highThreshold = *highThresholdP;

    // FIXME: Optimize for SSE2.
#if HAVE(ARM_NEON_INTRINSICS)
    if ((sourceStride == 1) && (destStride == 1)) {
        int tailFrames = n % 4;
        const float* endP = destP + n - tailFrames;

        float32x4_t low = vdupq_n_f32(lowThreshold);
        float32x4_t high = vdupq_n_f32(highThreshold);
        while (destP < endP) {
            float32x4_t source = vld1q_f32(sourceP);
            vst1q_f32(destP, vmaxq_f32(vminq_f32(source, high), low));
            sourceP += 4;
            destP += 4;
        }
        n = tailFrames;
    }
#endif
    while (n--) {
        *destP = std::max(std::min(*sourceP, highThreshold), lowThreshold);
        sourceP += sourceStride;
        destP += destStride;
    }
}

#endif // OS(DARWIN)

} // namespace VectorMath

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
