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

#ifndef DelayDSPKernel_h
#define DelayDSPKernel_h

#include "AudioArray.h"
#include "AudioDSPKernel.h"
#include "DelayProcessor.h"

namespace WebCore {

class DelayProcessor;
    
class DelayDSPKernel : public AudioDSPKernel {
public:  
    DelayDSPKernel(DelayProcessor*);
    DelayDSPKernel(double maxDelayTime, double sampleRate);
    
    virtual void process(const float* source, float* destination, size_t framesToProcess);
    virtual void reset();
    
    double maxDelayTime() const { return m_maxDelayTime; }
    
    void setDelayFrames(double numberOfFrames) { m_desiredDelayFrames = numberOfFrames; }
    
private:
    AudioFloatArray m_buffer;
    double m_maxDelayTime;
    int m_writeIndex;
    double m_currentDelayTime;
    double m_smoothingRate;
    bool m_firstTime;
    double m_desiredDelayFrames;

    DelayProcessor* delayProcessor() { return static_cast<DelayProcessor*>(processor()); }
};

} // namespace WebCore

#endif // DelayDSPKernel_h
