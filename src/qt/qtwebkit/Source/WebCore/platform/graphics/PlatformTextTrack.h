/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformTextTrack_h
#define PlatformTextTrack_h

#if USE(PLATFORM_TEXT_TRACK_MENU)

#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class TextTrack;
class InbandTextTrackPrivate;

class PlatformTextTrackClient {
public:
    virtual ~PlatformTextTrackClient() { }
    
    virtual TextTrack* publicTrack() = 0;
    virtual InbandTextTrackPrivate* privateTrack() { return 0; }
};

class PlatformTextTrack : public RefCounted<PlatformTextTrack> {
public:
    enum TrackKind {
        Subtitle = 0,
        Caption = 1,
        Description = 2,
        Chapter = 3,
        MetaData = 4,
        Forced = 5,
    };
    enum TrackType {
        InBand = 0,
        OutOfBand = 1,
        Script = 2
    };

    static PassRefPtr<PlatformTextTrack> create(PlatformTextTrackClient* client, const String& label, const String& language, TrackKind kind, TrackType type, int uniqueId)
    {
        return adoptRef(new PlatformTextTrack(client, label, language, kind, type, uniqueId));
    }

    virtual ~PlatformTextTrack() { }
    
    TrackType type() const { return m_type; }
    TrackKind kind() const { return m_kind; }
    String label() const { return m_label; }
    String language() const { return m_language; }
    PlatformTextTrackClient* client() const { return m_client; }
    int uniqueId() const { return m_uniqueId; }

    static PlatformTextTrack* captionMenuOffItem()
    {
        DEFINE_STATIC_LOCAL(RefPtr<PlatformTextTrack>, off, (PlatformTextTrack::create(0, "off menu item", "", Subtitle, InBand, 0)));
        return off.get();
    }

    static PlatformTextTrack* captionMenuAutomaticItem()
    {
        DEFINE_STATIC_LOCAL(RefPtr<PlatformTextTrack>, automatic, (PlatformTextTrack::create(0, "automatic menu item", "", Subtitle, InBand, 0)));
        return automatic.get();
    }

protected:
    PlatformTextTrack(PlatformTextTrackClient* client, const String& label, const String& language, TrackKind kind, TrackType type, int uniqueId)
        : m_label(label)
        , m_language(language)
        , m_kind(kind)
        , m_type(type)
        , m_client(client)
        , m_uniqueId(uniqueId)
    {
    }

    String m_label;
    String m_language;
    TrackKind m_kind;
    TrackType m_type;
    PlatformTextTrackClient* m_client;
    int m_uniqueId;
};

}

#endif

#endif // PlatformTextTrack_h
