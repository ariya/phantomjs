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

#ifndef RealtimeAnalyser_h
#define RealtimeAnalyser_h

#include "AudioArray.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class AudioBus;
class FFTFrame;

class RealtimeAnalyser {
    WTF_MAKE_NONCOPYABLE(RealtimeAnalyser);
public:
    RealtimeAnalyser();
    virtual ~RealtimeAnalyser();
    
    void reset();

    size_t fftSize() const { return m_fftSize; }
    bool setFftSize(size_t);

    unsigned frequencyBinCount() const { return m_fftSize / 2; }

    void setMinDecibels(float k) { m_minDecibels = k; }
    float minDecibels() const { return static_cast<float>(m_minDecibels); }

    void setMaxDecibels(float k) { m_maxDecibels = k; }
    float maxDecibels() const { return static_cast<float>(m_maxDecibels); }

    void setSmoothingTimeConstant(float k) { m_smoothingTimeConstant = k; }
    float smoothingTimeConstant() const { return static_cast<float>(m_smoothingTimeConstant); }

    void getFloatFrequencyData(Float32Array*);
    void getByteFrequencyData(Uint8Array*);
    void getByteTimeDomainData(Uint8Array*);

    // The audio thread writes input data here.
    void writeInput(AudioBus*, size_t framesToProcess);

    static const double DefaultSmoothingTimeConstant;
    static const double DefaultMinDecibels;
    static const double DefaultMaxDecibels;

    static const unsigned DefaultFFTSize;
    static const unsigned MinFFTSize;
    static const unsigned MaxFFTSize;
    static const unsigned InputBufferSize;

private:
    // The audio thread writes the input audio here.
    AudioFloatArray m_inputBuffer;
    unsigned m_writeIndex;
    
    size_t m_fftSize;
    OwnPtr<FFTFrame> m_analysisFrame;
    void doFFTAnalysis();
    
    // doFFTAnalysis() stores the floating-point magnitude analysis data here.
    AudioFloatArray m_magnitudeBuffer;
    AudioFloatArray& magnitudeBuffer() { return m_magnitudeBuffer; }

    // A value between 0 and 1 which averages the previous version of m_magnitudeBuffer with the current analysis magnitude data.
    double m_smoothingTimeConstant;    

    // The range used when converting when using getByteFrequencyData(). 
    double m_minDecibels;
    double m_maxDecibels;
};

} // namespace WebCore

#endif // RealtimeAnalyser_h
