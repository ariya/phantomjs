/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef AudioScheduledSourceNode_h
#define AudioScheduledSourceNode_h

#include "AudioNode.h"
#include "ExceptionCode.h"

namespace WebCore {

class AudioBus;

class AudioScheduledSourceNode : public AudioNode {
public:
    // These are the possible states an AudioScheduledSourceNode can be in:
    //
    // UNSCHEDULED_STATE - Initial playback state. Created, but not yet scheduled.
    // SCHEDULED_STATE - Scheduled to play (via noteOn() or noteGrainOn()), but not yet playing.
    // PLAYING_STATE - Generating sound.
    // FINISHED_STATE - Finished generating sound.
    //
    // The state can only transition to the next state, except for the FINISHED_STATE which can
    // never be changed.
    enum PlaybackState {
        // These must be defined with the same names and values as in the .idl file.
        UNSCHEDULED_STATE = 0,
        SCHEDULED_STATE = 1,
        PLAYING_STATE = 2,
        FINISHED_STATE = 3
    };
    
    AudioScheduledSourceNode(AudioContext*, float sampleRate);

    // Scheduling.
    void start(double when, ExceptionCode&);
    void stop(double when, ExceptionCode&);

#if ENABLE(LEGACY_WEB_AUDIO)
    void noteOn(double when, ExceptionCode&);
    void noteOff(double when, ExceptionCode&);
#endif

    unsigned short playbackState() const { return static_cast<unsigned short>(m_playbackState); }
    bool isPlayingOrScheduled() const { return m_playbackState == PLAYING_STATE || m_playbackState == SCHEDULED_STATE; }
    bool hasFinished() const { return m_playbackState == FINISHED_STATE; }

    EventListener* onended() { return getAttributeEventListener(eventNames().endedEvent); }
    void setOnended(PassRefPtr<EventListener> listener);

protected:
    // Get frame information for the current time quantum.
    // We handle the transition into PLAYING_STATE and FINISHED_STATE here,
    // zeroing out portions of the outputBus which are outside the range of startFrame and endFrame.
    //
    // Each frame time is relative to the context's currentSampleFrame().
    // quantumFrameOffset    : Offset frame in this time quantum to start rendering.
    // nonSilentFramesToProcess : Number of frames rendering non-silence (will be <= quantumFrameSize).
    void updateSchedulingInfo(size_t quantumFrameSize,
                              AudioBus* outputBus,
                              size_t& quantumFrameOffset,
                              size_t& nonSilentFramesToProcess);

    // Called when we have no more sound to play or the noteOff() time has been reached.
    virtual void finish();

    static void notifyEndedDispatch(void*);
    void notifyEnded();

    PlaybackState m_playbackState;

    // m_startTime is the time to start playing based on the context's timeline (0 or a time less than the context's current time means "now").
    double m_startTime; // in seconds

    // m_endTime is the time to stop playing based on the context's timeline (0 or a time less than the context's current time means "now").
    // If it hasn't been set explicitly, then the sound will not stop playing (if looping) or will stop when the end of the AudioBuffer
    // has been reached.
    double m_endTime; // in seconds

    bool m_hasEndedListener;

    static const double UnknownTime;
};

} // namespace WebCore

#endif // AudioScheduledSourceNode_h
