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

#include "VideoTrackList.h"

#include "EventNames.h"
#include "VideoTrack.h"

using namespace WebCore;

VideoTrackList::VideoTrackList(HTMLMediaElement* element, ScriptExecutionContext* context)
    : TrackListBase(element, context)
{
}

VideoTrackList::~VideoTrackList()
{
}

void VideoTrackList::append(PassRefPtr<VideoTrack> prpTrack)
{
    RefPtr<VideoTrack> track = prpTrack;

    // Insert tracks in the media file order.
    size_t index = track->inbandTrackIndex();
    m_inbandTracks.insert(index, track);

    ASSERT(!track->mediaElement() || track->mediaElement() == mediaElement());
    track->setMediaElement(mediaElement());

    scheduleAddTrackEvent(track.release());
}

VideoTrack* VideoTrackList::item(unsigned index) const
{
    if (index < m_inbandTracks.size())
        return toVideoTrack(m_inbandTracks[index].get());

    return 0;
}

VideoTrack* VideoTrackList::getTrackById(const AtomicString& id) const
{
    for (size_t i = 0; i < length(); ++i) {
        VideoTrack* track = toVideoTrack(m_inbandTracks[i].get());
        if (track->id() == id)
            return track;
    }
    return 0;
}

long VideoTrackList::selectedIndex() const
{
    // 4.8.10.10.1 AudioTrackList and VideoTrackList objects
    // The VideoTrackList.selectedIndex attribute must return the index of the
    // currently selected track, if any. If the VideoTrackList object does not
    // currently represent any tracks, or if none of the tracks are selected,
    // it must instead return −1.
    for (size_t i = 0; i < length(); ++i) {
        VideoTrack* track = toVideoTrack(m_inbandTracks[i].get());
        if (track->selected())
            return i;
    }
    return -1;
}

const AtomicString& VideoTrackList::interfaceName() const
{
    return eventNames().interfaceForVideoTrackList;
}

#endif
