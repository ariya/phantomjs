/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef InbandTextTrackPrivateAVFObjC_h
#define InbandTextTrackPrivateAVFObjC_h

#if ENABLE(VIDEO) && USE(AVFOUNDATION) && HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)

#include "InbandTextTrackPrivateAVF.h"
#include <wtf/RetainPtr.h>

OBJC_CLASS AVMediaSelectionOption;

namespace WebCore {

class InbandTextTrackPrivateAVFObjC : public InbandTextTrackPrivateAVF {
public:
    static PassRefPtr<InbandTextTrackPrivateAVFObjC> create(AVFInbandTrackParent* player,  AVMediaSelectionOption *selection)
    {
        return adoptRef(new InbandTextTrackPrivateAVFObjC(player, selection));
    }

    ~InbandTextTrackPrivateAVFObjC() { }

    virtual InbandTextTrackPrivate::Kind kind() const OVERRIDE;
    virtual bool isClosedCaptions() const OVERRIDE;
    virtual bool isSDH() const OVERRIDE;
    virtual bool containsOnlyForcedSubtitles() const OVERRIDE;
    virtual bool isMainProgramContent() const OVERRIDE;
    virtual bool isEasyToRead() const OVERRIDE;
    virtual AtomicString label() const OVERRIDE;
    virtual AtomicString language() const OVERRIDE;
    virtual bool isDefault() const OVERRIDE;

    virtual void disconnect() OVERRIDE;

    virtual bool isLegacyClosedCaptionsTrack() const OVERRIDE { return false; }

    AVMediaSelectionOption *mediaSelectionOption() const { return m_mediaSelectionOption.get(); }

protected:
    InbandTextTrackPrivateAVFObjC(AVFInbandTrackParent*, AVMediaSelectionOption *);
    
    RetainPtr<AVMediaSelectionOption> m_mediaSelectionOption;
};

}

#endif
#endif
