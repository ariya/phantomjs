/*
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FrameNetworkingContextWinCE_h
#define FrameNetworkingContextWinCE_h

#include "FrameNetworkingContext.h"

namespace WebKit {

class FrameNetworkingContextWinCE : public WebCore::FrameNetworkingContext {
public:
    static PassRefPtr<FrameNetworkingContextWinCE> create(WebCore::Frame* frame, const WTF::String& userAgent)
    {
        return adoptRef(new FrameNetworkingContextWinCE(frame, userAgent));
    }

    virtual WTF::String userAgent() const;
    virtual WTF::String referrer() const;
    virtual WebCore::ResourceError blockedError(const WebCore::ResourceRequest&) const;

private:
    FrameNetworkingContextWinCE(WebCore::Frame* frame, const WTF::String& userAgent);

    WTF::String m_userAgent;
};

} // namespace WebKit

#endif // FrameNetworkingContextWinCE_h
