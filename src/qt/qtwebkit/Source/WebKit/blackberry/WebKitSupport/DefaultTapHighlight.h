/*
 * Copyright (C) 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DefaultTapHighlight_h
#define DefaultTapHighlight_h

#include "BlackBerryGlobal.h"

#if USE(ACCELERATED_COMPOSITING)

#include "Color.h"
#include "GraphicsLayerClient.h"
#include "WebOverlay.h"
#include "WebTapHighlight.h"

#include <BlackBerryPlatformIntRectRegion.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Threading.h>

namespace BlackBerry {
namespace WebKit {

class WebPagePrivate;

class DefaultTapHighlight : public WebTapHighlight, public WebCore::GraphicsLayerClient {
public:
    static PassOwnPtr<DefaultTapHighlight> create(WebPagePrivate* page)
    {
        return adoptPtr(new DefaultTapHighlight(page));
    }

    virtual ~DefaultTapHighlight();

    virtual void draw(const Platform::IntRectRegion&, int red, int green, int blue, int alpha, bool hideAfterScroll, bool isStartOfSelection = false);
    virtual void hide();

    virtual bool isVisible() const { return m_visible; }
    virtual bool shouldHideAfterScroll() const { return m_shouldHideAfterScroll; }

    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double) { }
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& inClip);

private:
    DefaultTapHighlight(WebPagePrivate*);

    WebPagePrivate* m_page;
    OwnPtr<WebOverlay> m_overlay;
    BlackBerry::Platform::IntRectRegion m_region;
    WebCore::Color m_color;
    bool m_visible;
    bool m_shouldHideAfterScroll;
    Mutex m_mutex;
};

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)

#endif // DefaultTapHighlight_h
