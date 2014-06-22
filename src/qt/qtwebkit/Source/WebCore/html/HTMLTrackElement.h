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

#ifndef HTMLTrackElement_h
#define HTMLTrackElement_h

#if ENABLE(VIDEO_TRACK)
#include "HTMLElement.h"
#include "LoadableTextTrack.h"
#include "TextTrack.h"

namespace WebCore {

class HTMLMediaElement;

class HTMLTrackElement FINAL : public HTMLElement, public TextTrackClient {
public:
    static PassRefPtr<HTMLTrackElement> create(const QualifiedName&, Document*);

    KURL src() const;
    void setSrc(const String&);

    String kind();
    void setKind(const String&);

    String srclang() const;
    void setSrclang(const String&);

    String label() const;
    void setLabel(const String&);

    bool isDefault() const;
    void setIsDefault(bool);

    enum ReadyState { NONE = 0, LOADING = 1, LOADED = 2, TRACK_ERROR = 3 };
    ReadyState readyState();
    void setReadyState(ReadyState);

    TextTrack* track();

    void scheduleLoad();

    enum LoadStatus { Failure, Success };
    virtual void didCompleteLoad(LoadableTextTrack*, LoadStatus);

    const AtomicString& mediaElementCrossOriginAttribute() const;

private:
    HTMLTrackElement(const QualifiedName&, Document*);
    virtual ~HTMLTrackElement();

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;

    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;

    void loadTimerFired(Timer<HTMLTrackElement>*);

#if ENABLE(MICRODATA)
    virtual String itemValueText() const OVERRIDE;
    virtual void setItemValueText(const String&, ExceptionCode&) OVERRIDE;
#endif

    HTMLMediaElement* mediaElement() const;

    // TextTrackClient
    virtual void textTrackModeChanged(TextTrack*);
    virtual void textTrackKindChanged(TextTrack*);
    virtual void textTrackAddCues(TextTrack*, const TextTrackCueList*);
    virtual void textTrackRemoveCues(TextTrack*, const TextTrackCueList*);
    virtual void textTrackAddCue(TextTrack*, PassRefPtr<TextTrackCue>);
    virtual void textTrackRemoveCue(TextTrack*, PassRefPtr<TextTrackCue>);

    LoadableTextTrack* ensureTrack();
    virtual bool canLoadUrl(const KURL&);

    RefPtr<LoadableTextTrack> m_track;
    Timer<HTMLTrackElement> m_loadTimer;
};

}

#endif
#endif
