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

#ifndef InbandTextTrackPrivate_h
#define InbandTextTrackPrivate_h

#if ENABLE(VIDEO_TRACK)

#include "InbandTextTrackPrivateClient.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class InbandTextTrackPrivate : public RefCounted<InbandTextTrackPrivate> {
    WTF_MAKE_NONCOPYABLE(InbandTextTrackPrivate); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<InbandTextTrackPrivate> create()
    {
        return adoptRef(new InbandTextTrackPrivate());
    }
    virtual ~InbandTextTrackPrivate() { }

    void setClient(InbandTextTrackPrivateClient* client) { m_client = client; }
    InbandTextTrackPrivateClient* client() { return m_client; }

    enum Mode {
        Disabled,
        Hidden,
        Showing
    };
    virtual void setMode(Mode mode) { m_mode = mode; };
    virtual InbandTextTrackPrivate::Mode mode() const { return m_mode; }

    enum Kind {
        Subtitles,
        Captions,
        Descriptions,
        Chapters,
        Metadata,
        Forced,
        None
    };
    virtual Kind kind() const { return Subtitles; }
    virtual bool isClosedCaptions() const { return false; }
    virtual bool isSDH() const { return false; }
    virtual bool containsOnlyForcedSubtitles() const { return false; }
    virtual bool isMainProgramContent() const { return true; }
    virtual bool isEasyToRead() const { return false; }

    virtual AtomicString label() const { return emptyAtom; }
    virtual AtomicString language() const { return emptyAtom; }
    virtual bool isDefault() const { return false; }

    virtual int textTrackIndex() const { return 0; }

    void willBeRemoved()
    {
        if (m_client)
            m_client->willRemoveTextTrackPrivate(this);
    }

protected:
    InbandTextTrackPrivate()
        : m_client(0)
        , m_mode(Disabled)
    {
    }

private:
    InbandTextTrackPrivateClient* m_client;
    Mode m_mode;
};

} // namespace WebCore

#endif
#endif
