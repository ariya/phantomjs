/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FFTFrame_h
#define FFTFrame_h

#include "AudioArray.h"

#if OS(DARWIN) && !USE(WEBAUDIO_FFMPEG)
#define USE_ACCELERATE_FFT 1
#else
#define USE_ACCELERATE_FFT 0
#endif

#if USE_ACCELERATE_FFT
#include <Accelerate/Accelerate.h>
#endif

#if !USE_ACCELERATE_FFT

#if USE(WEBAUDIO_MKL)
#include "mkl_dfti.h"
#endif // USE(WEBAUDIO_MKL)

#if USE(WEBAUDIO_FFTW)
#include "fftw3.h"
#endif // USE(WEBAUDIO_FFTW)

#if USE(WEBAUDIO_FFMPEG)
struct RDFTContext;
#endif // USE(WEBAUDIO_FFMPEG)

#endif // !USE_ACCELERATE_FFT

#include <wtf/PassOwnPtr.h>
#include <wtf/Platform.h>
#include <wtf/Threading.h>

namespace WebCore {

// Defines the interface for an "FFT frame", an object which is able to perform a forward
// and reverse FFT, internally storing the resultant frequency-domain data.

class FFTFrame {
public:
    // The constructors, destructor, and methods up to the CROSS-PLATFORM section have platform-dependent implementations.

    FFTFrame(unsigned fftSize);
    FFTFrame(); // creates a blank/empty frame for later use with createInterpolatedFrame()
    FFTFrame(const FFTFrame& frame);
    ~FFTFrame();

    static void initialize();
    static void cleanup();
    void doFFT(float* data);
    void doInverseFFT(float* data);
    void multiply(const FFTFrame& frame); // multiplies ourself with frame : effectively operator*=()

    float* realData() const;
    float* imagData() const;

    void print(); // for debugging

    // CROSS-PLATFORM
    // The remaining public methods have cross-platform implementations:

    // Interpolates from frame1 -> frame2 as x goes from 0.0 -> 1.0
    static PassOwnPtr<FFTFrame> createInterpolatedFrame(const FFTFrame& frame1, const FFTFrame& frame2, double x);

    void doPaddedFFT(float* data, size_t dataSize); // zero-padding with dataSize <= fftSize
    double extractAverageGroupDelay();
    void addConstantGroupDelay(double sampleFrameDelay);

    unsigned fftSize() const { return m_FFTSize; }
    unsigned log2FFTSize() const { return m_log2FFTSize; }

private:
    unsigned m_FFTSize;
    unsigned m_log2FFTSize;

    void interpolateFrequencyComponents(const FFTFrame& frame1, const FFTFrame& frame2, double x);

#if USE_ACCELERATE_FFT
    DSPSplitComplex& dspSplitComplex() { return m_frame; }
    DSPSplitComplex dspSplitComplex() const { return m_frame; }

    static FFTSetup fftSetupForSize(unsigned fftSize);

    static FFTSetup* fftSetups;

    FFTSetup m_FFTSetup;

    DSPSplitComplex m_frame;
    AudioFloatArray m_realData;
    AudioFloatArray m_imagData;
#else // !USE_ACCELERATE_FFT

#if USE(WEBAUDIO_MKL)
    // Interleaves the planar real and imaginary data and returns a
    // pointer to the resulting storage which can be used for in-place
    // or out-of-place operations. FIXME: ideally all of the MKL
    // routines would operate on planar data and this method would be
    // removed.
    float* getUpToDateComplexData();

    static DFTI_DESCRIPTOR_HANDLE descriptorHandleForSize(unsigned fftSize);

    static DFTI_DESCRIPTOR_HANDLE* descriptorHandles;

    DFTI_DESCRIPTOR_HANDLE m_handle;
    AudioFloatArray m_complexData;
    AudioFloatArray m_realData;
    AudioFloatArray m_imagData;
#endif // USE(WEBAUDIO_MKL)

#if USE(WEBAUDIO_FFMPEG)
    static RDFTContext* contextForSize(unsigned fftSize, int trans);

    RDFTContext* m_forwardContext;
    RDFTContext* m_inverseContext;

    float* getUpToDateComplexData();
    AudioFloatArray m_complexData;
    AudioFloatArray m_realData;
    AudioFloatArray m_imagData;
#endif // USE(WEBAUDIO_FFMPEG)

#if USE(WEBAUDIO_FFTW)
    fftwf_plan m_forwardPlan;
    fftwf_plan m_backwardPlan;

    enum Direction {
        Forward,
        Backward
    };

    // Both the real and imaginary data are stored here.
    // The real data is stored first, followed by three float values of padding.
    // The imaginary data is stored after the padding and is 16-byte aligned (if m_data itself is aligned).
    // The reason we don't use separate arrays for real and imaginary is because the FFTW plans are shared
    // between FFTFrame instances and require that the real and imaginary data pointers be the same distance apart.
    AudioFloatArray m_data;

    static Mutex *s_planLock;
    static fftwf_plan* fftwForwardPlans;
    static fftwf_plan* fftwBackwardPlans;

    static fftwf_plan fftwPlanForSize(unsigned fftSize, Direction,
                                      float*, float*, float*);
#endif // USE(WEBAUDIO_FFTW)

#endif // !USE_ACCELERATE_FFT
};

} // namespace WebCore

#endif // FFTFrame_h
