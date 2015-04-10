/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef VideoTrackPrivate_h
#define VideoTrackPrivate_h

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicString.h>

#if ENABLE(VIDEO_TRACK)

namespace WebCore {

class VideoTrackPrivate;

class VideoTrackPrivateClient {
public:
    virtual ~VideoTrackPrivateClient() { }
    virtual void willRemoveVideoTrackPrivate(VideoTrackPrivate*) = 0;
};

class VideoTrackPrivate : public RefCounted<VideoTrackPrivate> {
    WTF_MAKE_NONCOPYABLE(VideoTrackPrivate); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<VideoTrackPrivate> create()
    {
        return adoptRef(new VideoTrackPrivate());
    }
    virtual ~VideoTrackPrivate() { }

    void setClient(VideoTrackPrivateClient* client) { m_client = client; }
    VideoTrackPrivateClient* client() { return m_client; }

    virtual void setSelected(bool selected) { m_selected = selected; };
    virtual bool selected() const { return m_selected; }

    enum Kind { Alternative, Captions, Main, Sign, Subtitles, Commentary, None };
    virtual Kind kind() const { return None; }

    virtual AtomicString id() const { return emptyAtom; }
    virtual AtomicString label() const { return emptyAtom; }
    virtual AtomicString language() const { return emptyAtom; }

    virtual int videoTrackIndex() const { return 0; }

    void willBeRemoved()
    {
        if (m_client)
            m_client->willRemoveVideoTrackPrivate(this);
    }

protected:
    VideoTrackPrivate()
        : m_selected(false)
    {
    }

private:
    VideoTrackPrivateClient* m_client;
    bool m_selected;
};

} // namespace WebCore

#endif
#endif
