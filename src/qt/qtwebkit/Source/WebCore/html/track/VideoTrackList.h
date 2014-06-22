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

#ifndef VideoTrackList_h
#define VideoTrackList_h

#if ENABLE(VIDEO_TRACK)

#include "TrackListBase.h"

namespace WebCore {

class VideoTrack;

class VideoTrackList : public TrackListBase {
public:
    static PassRefPtr<VideoTrackList> create(HTMLMediaElement* owner, ScriptExecutionContext* context)
    {
        return adoptRef(new VideoTrackList(owner, context));
    }
    ~VideoTrackList();

    VideoTrack* getTrackById(const AtomicString&) const;
    long selectedIndex() const;

    VideoTrack* item(unsigned) const;
    VideoTrack* lastItem() const { return item(length() - 1); }
    void append(PassRefPtr<VideoTrack>);

    // EventTarget
    virtual const AtomicString& interfaceName() const OVERRIDE;

private:
    VideoTrackList(HTMLMediaElement*, ScriptExecutionContext*);

};

} // namespace WebCore

#endif
#endif
