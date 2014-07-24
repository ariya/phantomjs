/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef CachedPage_h
#define CachedPage_h

#include "CachedFrame.h"
#include <wtf/RefCounted.h>

namespace WebCore {
    
class Document;
class DocumentLoader;
class Page;

class CachedPage : public RefCounted<CachedPage> {
public:
    static PassRefPtr<CachedPage> create(Page*);
    ~CachedPage();

    void restore(Page*);
    void clear();
    void destroy();

    Document* document() const { return m_cachedMainFrame->document(); }
    DocumentLoader* documentLoader() const { return m_cachedMainFrame->documentLoader(); }

    double timeStamp() const { return m_timeStamp; }
    bool hasExpired() const;
    
    CachedFrame* cachedMainFrame() { return m_cachedMainFrame.get(); }

    void markForVistedLinkStyleRecalc() { m_needStyleRecalcForVisitedLinks = true; }
    void markForFullStyleRecalc() { m_needsFullStyleRecalc = true; }
#if ENABLE(VIDEO_TRACK)
    void markForCaptionPreferencesChanged() { m_needsCaptionPreferencesChanged = true; }
#endif

#if USE(ACCELERATED_COMPOSITING)
    void markForDeviceScaleChanged() { m_needsDeviceScaleChanged = true; }
#endif

private:
    CachedPage(Page*);

    double m_timeStamp;
    double m_expirationTime;
    RefPtr<CachedFrame> m_cachedMainFrame;
    bool m_needStyleRecalcForVisitedLinks;
    bool m_needsFullStyleRecalc;
    bool m_needsCaptionPreferencesChanged;
    bool m_needsDeviceScaleChanged;
};

} // namespace WebCore

#endif // CachedPage_h

