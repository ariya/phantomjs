/*
 * Copyright (C) 2011, 2012 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "AudioTrackList.h"

#include "AudioTrack.h"
#include "EventNames.h"

using namespace WebCore;

AudioTrackList::AudioTrackList(HTMLMediaElement* element, ScriptExecutionContext* context)
    : TrackListBase(element, context)
{
}

AudioTrackList::~AudioTrackList()
{
}

void AudioTrackList::append(PassRefPtr<AudioTrack> prpTrack)
{
    RefPtr<AudioTrack> track = prpTrack;

    // Insert tracks in the media file order.
    size_t index = track->inbandTrackIndex();
    m_inbandTracks.insert(index, track);

    ASSERT(!track->mediaElement() || track->mediaElement() == mediaElement());
    track->setMediaElement(mediaElement());

    scheduleAddTrackEvent(track.release());
}

AudioTrack* AudioTrackList::item(unsigned index) const
{
    if (index < m_inbandTracks.size())
        return toAudioTrack(m_inbandTracks[index].get());

    return 0;
}

AudioTrack* AudioTrackList::getTrackById(const AtomicString& id) const
{
    for (size_t i = 0; i < m_inbandTracks.size(); ++i) {
        AudioTrack* track = toAudioTrack(m_inbandTracks[i].get());
        if (track->id() == id)
            return track;
    }
    return 0;
}

const AtomicString& AudioTrackList::interfaceName() const
{
    return eventNames().interfaceForAudioTrackList;
}

#endif
