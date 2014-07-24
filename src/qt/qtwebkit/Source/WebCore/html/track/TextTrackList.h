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

#ifndef TextTrackList_h
#define TextTrackList_h

#if ENABLE(VIDEO_TRACK)

#include "TrackListBase.h"

namespace WebCore {

class TextTrack;

class TextTrackList : public TrackListBase {
public:
    static PassRefPtr<TextTrackList> create(HTMLMediaElement* element, ScriptExecutionContext* context)
    {
        return adoptRef(new TextTrackList(element, context));
    }
    ~TextTrackList();

    virtual unsigned length() const OVERRIDE;
    int getTrackIndex(TextTrack*);
    int getTrackIndexRelativeToRenderedTracks(TextTrack*);
    virtual bool contains(TrackBase*) const OVERRIDE;

    TextTrack* item(unsigned index);
    void append(PassRefPtr<TextTrack>);
    virtual void remove(TrackBase*) OVERRIDE;

    // EventTarget
    virtual const AtomicString& interfaceName() const OVERRIDE;

private:
    TextTrackList(HTMLMediaElement*, ScriptExecutionContext*);

    void invalidateTrackIndexesAfterTrack(TextTrack*);

    Vector<RefPtr<TrackBase> > m_addTrackTracks;
    Vector<RefPtr<TrackBase> > m_elementTracks;
};

} // namespace WebCore

#endif
#endif
