/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef FindController_h
#define FindController_h

#include "PageOverlay.h"
#include "ShareableBitmap.h"
#include "WebFindOptions.h"
#include <WebCore/IntRect.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebCore {
    class Frame;
    class Range;
}

namespace WebKit {

class WebPage;

class FindController : private PageOverlay::Client {
    WTF_MAKE_NONCOPYABLE(FindController);

public:
    explicit FindController(WebPage*);
    virtual ~FindController();

    void findString(const String&, FindOptions, unsigned maxMatchCount);
    void findStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void getImageForFindMatch(uint32_t matchIndex);
    void selectFindMatch(uint32_t matchIndex);
    void hideFindUI();
    void countStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    
    void hideFindIndicator();
    void showFindIndicatorInSelection();

    bool isShowingOverlay() const { return m_isShowingFindIndicator && m_findPageOverlay; }

    void deviceScaleFactorDidChange();

private:
    // PageOverlay::Client.
    virtual void pageOverlayDestroyed(PageOverlay*);
    virtual void willMoveToWebPage(PageOverlay*, WebPage*);
    virtual void didMoveToWebPage(PageOverlay*, WebPage*);
    virtual bool mouseEvent(PageOverlay*, const WebMouseEvent&);
    virtual void drawRect(PageOverlay*, WebCore::GraphicsContext&, const WebCore::IntRect& dirtyRect);

    Vector<WebCore::IntRect> rectsForTextMatches();
    bool getFindIndicatorBitmapAndRect(WebCore::Frame*, ShareableBitmap::Handle&, WebCore::IntRect& selectionRect);
    bool updateFindIndicator(WebCore::Frame* selectedFrame, bool isShowingOverlay, bool shouldAnimate = true);

    void updateFindUIAfterPageScroll(bool found, const String&, FindOptions, unsigned maxMatchCount);

    WebPage* m_webPage;
    PageOverlay* m_findPageOverlay;

    // Whether the UI process is showing the find indicator. Note that this can be true even if
    // the find indicator isn't showing, but it will never be false when it is showing.
    bool m_isShowingFindIndicator;
    WebCore::IntRect m_findIndicatorRect;
    Vector<RefPtr<WebCore::Range> > m_findMatches;
};

} // namespace WebKit

#endif // FindController_h
