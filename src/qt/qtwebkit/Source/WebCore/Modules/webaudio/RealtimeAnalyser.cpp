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

#include "RealtimeAnalyser.h"

#include "AudioBus.h"
#include "AudioUtilities.h"
#include "FFTFrame.h"
#include "VectorMath.h"
#include <algorithm>
#include <complex>
#include <wtf/Float32Array.h>
#include <wtf/MainThread.h>
#include <wtf/MathExtras.h>
#include <wtf/Uint8Array.h>

namespace WebCore {

const double RealtimeAnalyser::DefaultSmoothingTimeConstant  = 0.8;
const double RealtimeAnalyser::DefaultMinDecibels = -100;
const double RealtimeAnalyser::DefaultMaxDecibels = -30;

const unsigned RealtimeAnalyser::DefaultFFTSize = 2048;
// All FFT implementations are expected to handle power-of-two sizes MinFFTSize <= size <= MaxFFTSize.
const unsigned RealtimeAnalyser::MinFFTSize = 32;
const unsigned RealtimeAnalyser::MaxFFTSize = 2048;
const unsigned RealtimeAnalyser::InputBufferSize = RealtimeAnalyser::MaxFFTSize * 2;

RealtimeAnalyser::RealtimeAnalyser()
    : m_inputBuffer(InputBufferSize)
    , m_writeIndex(0)
    , m_fftSize(DefaultFFTSize)
    , m_magnitudeBuffer(DefaultFFTSize / 2)
    , m_smoothingTimeConstant(DefaultSmoothingTimeConstant)
    , m_minDecibels(DefaultMinDecibels)
    , m_maxDecibels(DefaultMaxDecibels)
{
    m_analysisFrame = adoptPtr(new FFTFrame(DefaultFFTSize));
}

RealtimeAnalyser::~RealtimeAnalyser()
{
}

void RealtimeAnalyser::reset()
{
    m_writeIndex = 0;
    m_inputBuffer.zero();
    m_magnitudeBuffer.zero();
}

bool RealtimeAnalyser::setFftSize(size_t size)
{
    ASSERT(isMainThread());

    // Only allow powers of two.
    unsigned log2size = static_cast<unsigned>(log2(size));
    bool isPOT(1UL << log2size == size);

    if (!isPOT || size > MaxFFTSize || size < MinFFTSize)
        return false;

    if (m_fftSize != size) {
        m_analysisFrame = adoptPtr(new FFTFrame(size));
        // m_magnitudeBuffer has size = fftSize / 2 because it contains floats reduced from complex values in m_analysisFrame.
        m_magnitudeBuffer.allocate(size / 2);
        m_fftSize = size;
    }

    return true;
}

void RealtimeAnalyser::writeInput(AudioBus* bus, size_t framesToProcess)
{
    bool isBusGood = bus && bus->numberOfChannels() > 0 && bus->channel(0)->length() >= framesToProcess;
    ASSERT(isBusGood);
    if (!isBusGood)
        return;
        
    // FIXME : allow to work with non-FFTSize divisible chunking
    bool isDestinationGood = m_writeIndex < m_inputBuffer.size() && m_writeIndex + framesToProcess <= m_inputBuffer.size();
    ASSERT(isDestinationGood);
    if (!isDestinationGood)
        return;    
    
    // Perform real-time analysis
    const float* source = bus->channel(0)->data();
    float* dest = m_inputBuffer.data() + m_writeIndex;

    // The source has already been sanity checked with isBusGood above.
    memcpy(dest, source, sizeof(float) * framesToProcess);

    // Sum all channels in one if numberOfChannels > 1.
    unsigned numberOfChannels = bus->numberOfChannels();
    if (numberOfChannels > 1) {
        for (unsigned i = 1; i < numberOfChannels; i++) {
            source = bus->channel(i)->data();
            VectorMath::vadd(dest, 1, source, 1, dest, 1, framesToProcess);
        }
        const float scale =  1.0 / numberOfChannels;
        VectorMath::vsmul(dest, 1, &scale, dest, 1, framesToProcess);
    }

    m_writeIndex += framesToProcess;
    if (m_writeIndex >= InputBufferSize)
        m_writeIndex = 0;
}

namespace {

void applyWindow(float* p, size_t n)
{
    ASSERT(isMainThread());
    
    // Blackman window
    double alpha = 0.16;
    double a0 = 0.5 * (1 - alpha);
    double a1 = 0.5;
    double a2 = 0.5 * alpha;
    
    for (unsigned i = 0; i < n; ++i) {
        double x = static_cast<double>(i) / static_cast<double>(n);
        double window = a0 - a1 * cos(2 * piDouble * x) + a2 * cos(4 * piDouble * x);
        p[i] *= float(window);
    }
}

} // namespace

void RealtimeAnalyser::doFFTAnalysis()
{    
    ASSERT(isMainThread());

    // Unroll the input buffer into a temporary buffer, where we'll apply an analysis window followed by an FFT.
    size_t fftSize = this->fftSize();
    
    AudioFloatArray temporaryBuffer(fftSize);
    float* inputBuffer = m_inputBuffer.data();
    float* tempP = temporaryBuffer.data();

    // Take the previous fftSize values from the input buffer and copy into the temporary buffer.
    unsigned writeIndex = m_writeIndex;
    if (writeIndex < fftSize) {
        memcpy(tempP, inputBuffer + writeIndex - fftSize + InputBufferSize, sizeof(*tempP) * (fftSize - writeIndex));
        memcpy(tempP + fftSize - writeIndex, inputBuffer, sizeof(*tempP) * writeIndex);
    } else 
        memcpy(tempP, inputBuffer + writeIndex - fftSize, sizeof(*tempP) * fftSize);

    
    // Window the input samples.
    applyWindow(tempP, fftSize);
    
    // Do the analysis.
    m_analysisFrame->doFFT(tempP);

    float* realP = m_analysisFrame->realData();
    float* imagP = m_analysisFrame->imagData();

    // Blow away the packed nyquist component.
    imagP[0] = 0;
    
    // Normalize so than an input sine wave at 0dBfs registers as 0dBfs (undo FFT scaling factor).
    const double magnitudeScale = 1.0 / DefaultFFTSize;

    // A value of 0 does no averaging with the previous result.  Larger values produce slower, but smoother changes.
    double k = m_smoothingTimeConstant;
    k = std::max(0.0, k);
    k = std::min(1.0, k);    
    
    // Convert the analysis data from complex to magnitude and average with the previous result.
    float* destination = magnitudeBuffer().data();
    size_t n = magnitudeBuffer().size();
    for (size_t i = 0; i < n; ++i) {
        std::complex<double> c(realP[i], imagP[i]);
        double scalarMagnitude = abs(c) * magnitudeScale;        
        destination[i] = static_cast<float>(k * destination[i] + (1 - k) * scalarMagnitude);
    }
}

void RealtimeAnalyser::getFloatFrequencyData(Float32Array* destinationArray)
{
    ASSERT(isMainThread());

    if (!destinationArray)
        return;
        
    doFFTAnalysis();
    
    // Convert from linear magnitude to floating-point decibels.
    const double minDecibels = m_minDecibels;
    unsigned sourceLength = magnitudeBuffer().size();
    size_t len = std::min(sourceLength, destinationArray->length());
    if (len > 0) {
        const float* source = magnitudeBuffer().data();
        float* destination = destinationArray->data();
        
        for (unsigned i = 0; i < len; ++i) {
            float linearValue = source[i];
            double dbMag = !linearValue ? minDecibels : AudioUtilities::linearToDecibels(linearValue);
            destination[i] = static_cast<float>(dbMag);
        }
    }
}

void RealtimeAnalyser::getByteFrequencyData(Uint8Array* destinationArray)
{
    ASSERT(isMainThread());

    if (!destinationArray)
        return;
        
    doFFTAnalysis();
    
    // Convert from linear magnitude to unsigned-byte decibels.
    unsigned sourceLength = magnitudeBuffer().size();
    size_t len = std::min(sourceLength, destinationArray->length());
    if (len > 0) {
        const double rangeScaleFactor = m_maxDecibels == m_minDecibels ? 1 : 1 / (m_maxDecibels - m_minDecibels);
        const double minDecibels = m_minDecibels;

        const float* source = magnitudeBuffer().data();
        unsigned char* destination = destinationArray->data();        
        
        for (unsigned i = 0; i < len; ++i) {
            float linearValue = source[i];
            double dbMag = !linearValue ? minDecibels : AudioUtilities::linearToDecibels(linearValue);
            
            // The range m_minDecibels to m_maxDecibels will be scaled to byte values from 0 to UCHAR_MAX.
            double scaledValue = UCHAR_MAX * (dbMag - minDecibels) * rangeScaleFactor;

            // Clip to valid range.
            if (scaledValue < 0)
                scaledValue = 0;
            if (scaledValue > UCHAR_MAX)
                scaledValue = UCHAR_MAX;
            
            destination[i] = static_cast<unsigned char>(scaledValue);
        }
    }
}

void RealtimeAnalyser::getByteTimeDomainData(Uint8Array* destinationArray)
{
    ASSERT(isMainThread());

    if (!destinationArray)
        return;
        
    unsigned fftSize = this->fftSize();
    size_t len = std::min(fftSize, destinationArray->length());
    if (len > 0) {
        bool isInputBufferGood = m_inputBuffer.size() == InputBufferSize && m_inputBuffer.size() > fftSize;
        ASSERT(isInputBufferGood);
        if (!isInputBufferGood)
            return;

        float* inputBuffer = m_inputBuffer.data();        
        unsigned char* destination = destinationArray->data();
        
        unsigned writeIndex = m_writeIndex;

        for (unsigned i = 0; i < len; ++i) {
            // Buffer access is protected due to modulo operation.
            float value = inputBuffer[(i + writeIndex - fftSize + InputBufferSize) % InputBufferSize];

            // Scale from nominal -1 -> +1 to unsigned byte.
            double scaledValue = 128 * (value + 1);

            // Clip to valid range.
            if (scaledValue < 0)
                scaledValue = 0;
            if (scaledValue > UCHAR_MAX)
                scaledValue = UCHAR_MAX;
            
            destination[i] = static_cast<unsigned char>(scaledValue);
        }
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
