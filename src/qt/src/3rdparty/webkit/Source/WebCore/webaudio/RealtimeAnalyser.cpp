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

#if ENABLE(WEBGL)
#include "Float32Array.h"
#include "Uint8Array.h"
#endif

#include <algorithm>
#include <limits.h>
#include <wtf/Complex.h>
#include <wtf/MathExtras.h>
#include <wtf/Threading.h>

using namespace std;

namespace WebCore {

const double RealtimeAnalyser::DefaultSmoothingTimeConstant  = 0.8;
const double RealtimeAnalyser::DefaultMinDecibels = -100.0;
const double RealtimeAnalyser::DefaultMaxDecibels = -30.0;

const unsigned RealtimeAnalyser::DefaultFFTSize = 2048;
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

void RealtimeAnalyser::setFftSize(size_t size)
{
    ASSERT(isMainThread());

    // Only allow powers of two.
    unsigned log2size = static_cast<unsigned>(log2(size));
    bool isPOT(1UL << log2size == size);
    
    if (!isPOT || size > MaxFFTSize) {
        // FIXME: It would be good to also set an exception.
        return;
    }

    if (m_fftSize != size) {
        m_analysisFrame = adoptPtr(new FFTFrame(m_fftSize));
        m_magnitudeBuffer.resize(size);
        m_fftSize = size;
    }
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
    // FIXME : for now just use left channel (must mix if stereo source)
    float* source = bus->channel(0)->data();

    // The source has already been sanity checked with isBusGood above.
    
    memcpy(m_inputBuffer.data() + m_writeIndex, source, sizeof(float) * framesToProcess);

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
    double a0 = 0.5 * (1.0 - alpha);
    double a1 = 0.5;
    double a2 = 0.5 * alpha;
    
    for (unsigned i = 0; i < n; ++i) {
        double x = static_cast<double>(i) / static_cast<double>(n);
        double window = a0 - a1 * cos(2.0 * piDouble * x) + a2 * cos(4.0 * piDouble * x);
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
    // FIXME : optimize with memcpy().
    unsigned writeIndex = m_writeIndex;
    for (unsigned i = 0; i < fftSize; ++i)
        tempP[i] = inputBuffer[(i + writeIndex - fftSize + InputBufferSize) % InputBufferSize];
    
    // Window the input samples.
    applyWindow(tempP, fftSize);
    
    // Do the analysis.
    m_analysisFrame->doFFT(tempP);

    size_t n = DefaultFFTSize / 2;

    float* realP = m_analysisFrame->realData();
    float* imagP = m_analysisFrame->imagData();

    // Blow away the packed nyquist component.
    imagP[0] = 0.0f;
    
    // Normalize so than an input sine wave at 0dBfs registers as 0dBfs (undo FFT scaling factor).
    const double MagnitudeScale = 1.0 / DefaultFFTSize;

    // A value of 0 does no averaging with the previous result.  Larger values produce slower, but smoother changes.
    double k = m_smoothingTimeConstant;
    k = max(0.0, k);
    k = min(1.0, k);    
    
    // Convert the analysis data from complex to magnitude and average with the previous result.
    float* destination = magnitudeBuffer().data();
    for (unsigned i = 0; i < n; ++i) {
        Complex c(realP[i], imagP[i]);
        double scalarMagnitude = abs(c) * MagnitudeScale;        
        destination[i] = float(k * destination[i] + (1.0 - k) * scalarMagnitude);
    }
}

#if ENABLE(WEBGL)

void RealtimeAnalyser::getFloatFrequencyData(Float32Array* destinationArray)
{
    ASSERT(isMainThread());

    if (!destinationArray)
        return;
        
    doFFTAnalysis();
    
    // Convert from linear magnitude to floating-point decibels.
    const double MinDecibels = m_minDecibels;
    unsigned sourceLength = magnitudeBuffer().size();
    size_t len = min(sourceLength, destinationArray->length());
    if (len > 0) {
        const float* source = magnitudeBuffer().data();
        float* destination = destinationArray->data();
        
        for (unsigned i = 0; i < len; ++i) {
            float linearValue = source[i];
            double dbMag = !linearValue ? MinDecibels : AudioUtilities::linearToDecibels(linearValue);
            destination[i] = float(dbMag);
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
    size_t len = min(sourceLength, destinationArray->length());
    if (len > 0) {
        const double RangeScaleFactor = m_maxDecibels == m_minDecibels ? 1.0 : 1.0 / (m_maxDecibels - m_minDecibels);

        const float* source = magnitudeBuffer().data();
        unsigned char* destination = destinationArray->data();        
        
        for (unsigned i = 0; i < len; ++i) {
            float linearValue = source[i];
            double dbMag = !linearValue ? m_minDecibels : AudioUtilities::linearToDecibels(linearValue);
            
            // The range m_minDecibels to m_maxDecibels will be scaled to byte values from 0 to UCHAR_MAX.
            double scaledValue = UCHAR_MAX * (dbMag - m_minDecibels) * RangeScaleFactor;

            // Clip to valid range.
            if (scaledValue < 0.0)
                scaledValue = 0.0;
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
    size_t len = min(fftSize, destinationArray->length());
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

            // Scale from nominal -1.0 -> +1.0 to unsigned byte.
            double scaledValue = 128.0 * (value + 1.0);

            // Clip to valid range.
            if (scaledValue < 0.0)
                scaledValue = 0.0;
            if (scaledValue > UCHAR_MAX)
                scaledValue = UCHAR_MAX;
            
            destination[i] = static_cast<unsigned char>(scaledValue);
        }
    }
}

#endif // WEBGL

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
