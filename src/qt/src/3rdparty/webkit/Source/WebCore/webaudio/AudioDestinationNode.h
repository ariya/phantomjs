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

#ifndef AudioDestinationNode_h
#define AudioDestinationNode_h

#include "AudioBuffer.h"
#include "AudioNode.h"
#include "AudioSourceProvider.h"

namespace WebCore {

class AudioBus;
class AudioContext;
    
class AudioDestinationNode : public AudioNode, public AudioSourceProvider {
public:
    AudioDestinationNode(AudioContext*, double sampleRate);
    virtual ~AudioDestinationNode();
    
    // AudioNode   
    virtual void process(size_t) { }; // we're pulled by hardware so this is never called
    virtual void reset() { m_currentTime = 0.0; };
    
    // The audio hardware calls here periodically to gets its input stream.
    virtual void provideInput(AudioBus*, size_t numberOfFrames);

    double currentTime() { return m_currentTime; }

    virtual double sampleRate() const = 0;

    virtual unsigned numberOfChannels() const { return 2; } // FIXME: update when multi-channel (more than stereo) is supported

    virtual void startRendering() = 0;
    
protected:
    double m_currentTime;
};

} // namespace WebCore

#endif // AudioDestinationNode_h
