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

#ifndef AudioDestination_h
#define AudioDestination_h

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AudioIOCallback;

// AudioDestination is an abstraction for audio hardware I/O.
// The audio hardware periodically calls the AudioIOCallback render() method asking it to render/output the next render quantum of audio.
// It optionally will pass in local/live audio input when it calls render().

class AudioDestination {
public:
    // Pass in (numberOfInputChannels > 0) if live/local audio input is desired.
    // Port-specific device identification information for live/local input streams can be passed in the inputDeviceId.
    static PassOwnPtr<AudioDestination> create(AudioIOCallback&, const String& inputDeviceId, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate);

    virtual ~AudioDestination() { }

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() = 0;

    // Sample-rate conversion may happen in AudioDestination to the hardware sample-rate
    virtual float sampleRate() const = 0;
    static float hardwareSampleRate();

    // maxChannelCount() returns the total number of output channels of the audio hardware.
    // A value of 0 indicates that the number of channels cannot be configured and
    // that only stereo (2-channel) destinations can be created.
    // The numberOfOutputChannels parameter of AudioDestination::create() is allowed to
    // be a value: 1 <= numberOfOutputChannels <= maxChannelCount(),
    // or if maxChannelCount() equals 0, then numberOfOutputChannels must be 2.
    static unsigned long maxChannelCount();
};

} // namespace WebCore

#endif // AudioDestination_h
