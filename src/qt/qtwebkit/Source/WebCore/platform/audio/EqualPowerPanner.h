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

#ifndef EqualPowerPanner_h
#define EqualPowerPanner_h

#include "Panner.h"

namespace WebCore {

// Common type of stereo panner as found in normal audio mixing equipment.

class EqualPowerPanner : public Panner {
public:
    EqualPowerPanner(float sampleRate);

    virtual void pan(double azimuth, double elevation, const AudioBus* inputBus, AudioBus* outputBuf, size_t framesToProcess);

    virtual void reset() { m_isFirstRender = true; }

    virtual double tailTime() const OVERRIDE { return 0; }
    virtual double latencyTime() const OVERRIDE { return 0; }

private:
    // For smoothing / de-zippering
    bool m_isFirstRender;
    double m_smoothingConstant;
    
    double m_gainL;
    double m_gainR;
};

} // namespace WebCore

#endif // EqualPowerPanner_h
