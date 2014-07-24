/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#ifndef SVGImageChromeClient_h
#define SVGImageChromeClient_h

#if ENABLE(SVG)

#include "EmptyClients.h"

namespace WebCore {

class SVGImageChromeClient : public EmptyChromeClient {
    WTF_MAKE_NONCOPYABLE(SVGImageChromeClient); WTF_MAKE_FAST_ALLOCATED;
public:
    SVGImageChromeClient(SVGImage* image)
        : m_image(image)
    {
    }
    
    virtual bool isSVGImageChromeClient() const { return true; }
    SVGImage* image() const { return m_image; }
    
private:
    virtual void chromeDestroyed()
    {
        m_image = 0;
    }
    
    virtual void invalidateContentsAndRootView(const IntRect& r, bool)
    {
        // If m_image->m_page is null, we're being destructed, don't fire changedInRect() in that case.
        if (m_image && m_image->imageObserver() && m_image->m_page)
            m_image->imageObserver()->changedInRect(m_image, r);
    }
    
    SVGImage* m_image;
};

inline SVGImageChromeClient* toSVGImageChromeClient(ChromeClient* client)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!client || client->isSVGImageChromeClient());
    return static_cast<SVGImageChromeClient*>(client);
}
    
} 

#endif // ENABLE(SVG)
#endif // SVGImageChromeClient_h
