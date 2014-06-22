/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef LoadableTextTrack_h
#define LoadableTextTrack_h

#if ENABLE(VIDEO_TRACK)

#include "TextTrack.h"
#include "TextTrackLoader.h"
#include <wtf/Vector.h>

namespace WebCore {

class HTMLTrackElement;
class LoadableTextTrack;

class LoadableTextTrackClient : public TextTrackClient {
public:
    virtual ~LoadableTextTrackClient() { }
    
    virtual bool canLoadUrl(LoadableTextTrack*, const KURL&) { return false; }
    virtual void loadingCompleted(LoadableTextTrack*, bool /* loadingFailed */) { }
};

class LoadableTextTrack : public TextTrack, private TextTrackLoaderClient {
public:
    static PassRefPtr<LoadableTextTrack> create(HTMLTrackElement* track, const String& kind, const String& label, const String& language)
    {
        return adoptRef(new LoadableTextTrack(track, kind, label, language));
    }
    virtual ~LoadableTextTrack();

    void scheduleLoad(const KURL&);

    virtual void clearClient();

    size_t trackElementIndex();
    HTMLTrackElement* trackElement() { return m_trackElement; }
    void setTrackElement(HTMLTrackElement*);
    virtual Element* element() OVERRIDE;

    virtual bool isDefault() const OVERRIDE { return m_isDefault; }
    virtual void setIsDefault(bool isDefault) OVERRIDE  { m_isDefault = isDefault; }

private:
    // TextTrackLoaderClient
    virtual bool shouldLoadCues(TextTrackLoader*) { return true; }
    virtual void newCuesAvailable(TextTrackLoader*);
    virtual void cueLoadingStarted(TextTrackLoader*);
    virtual void cueLoadingCompleted(TextTrackLoader*, bool loadingFailed);
#if ENABLE(WEBVTT_REGIONS)
    virtual void newRegionsAvailable(TextTrackLoader*);
#endif

    LoadableTextTrack(HTMLTrackElement*, const String& kind, const String& label, const String& language);

    void loadTimerFired(Timer<LoadableTextTrack>*);

    HTMLTrackElement* m_trackElement;
    Timer<LoadableTextTrack> m_loadTimer;
    OwnPtr<TextTrackLoader> m_loader;
    KURL m_url;
    bool m_isDefault;
};
} // namespace WebCore

#endif
#endif
