/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef TapHighlightController_h
#define TapHighlightController_h

#if ENABLE(TOUCH_EVENTS)

#include "PageOverlay.h"
#include <WebCore/Color.h>
#include <WebCore/Path.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebCore {
class Frame;
class IntRect;
class Node;
}

namespace WebKit {

class WebPage;

class TapHighlightController : private PageOverlay::Client {
    WTF_MAKE_NONCOPYABLE(TapHighlightController);

public:
    explicit TapHighlightController(WebPage*);
    virtual ~TapHighlightController();

    void highlight(WebCore::Node*);
    void hideHighlight();

private:
    // PageOverlay::Client.
    virtual void pageOverlayDestroyed(PageOverlay*);
    virtual void willMoveToWebPage(PageOverlay*, WebPage*);
    virtual void didMoveToWebPage(PageOverlay*, WebPage*);
    virtual bool mouseEvent(PageOverlay*, const WebMouseEvent&);
    virtual void drawRect(PageOverlay*, WebCore::GraphicsContext&, const WebCore::IntRect& dirtyRect);

private:
    WebPage* m_webPage;
    PageOverlay* m_overlay;

    WebCore::Path m_path;
    WebCore::Color m_color;
};

} // namespace WebKit

#endif // ENABLE(TOUCH_EVENTS)

#endif // TapHighlightController_h
