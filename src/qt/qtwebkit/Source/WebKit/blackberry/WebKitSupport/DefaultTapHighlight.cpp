/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
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

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "DefaultTapHighlight.h"

#include "GraphicsContext.h"
#include "Path.h"
#include "WebAnimation.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformGraphicsContext.h>
#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformPath.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

const double ActiveTextFadeAnimationDuration = 0.3;
const double OverlayShrinkAnimationDuration = 0.5;
const double OverlayInitialScale = 2.0;

STATIC_LOCAL_STRING(s_fadeAnimationName, "fade");
STATIC_LOCAL_STRING(s_shrinkAnimationName, "shrink");

DefaultTapHighlight::DefaultTapHighlight(WebPagePrivate* page)
    : m_page(page)
    , m_visible(false)
    , m_shouldHideAfterScroll(false)
{
}

DefaultTapHighlight::~DefaultTapHighlight()
{
}

void DefaultTapHighlight::draw(const Platform::IntRectRegion& region, int red, int green, int blue, int alpha, bool hideAfterScroll, bool isStartOfSelection)
{
    ASSERT(BlackBerry::Platform::webKitThreadMessageClient()->isCurrentThread());

    m_region = region;
    m_color = Color(red, green, blue, std::min(128, alpha));
    m_shouldHideAfterScroll = hideAfterScroll;
    FloatRect rect = IntRect(m_region.extents());
    if (rect.isEmpty())
        return;

    // Transparent color means disable the tap highlight.
    if (!m_color.alpha()) {
        hide();
        return;
    }

    {
        MutexLocker lock(m_mutex);
        m_visible = true;
    }

    if (!m_overlay) {
        m_overlay = adoptPtr(new WebOverlay(this));
        m_page->m_webPage->addOverlay(m_overlay.get());
    }

    m_overlay->removeAnimation(s_shrinkAnimationName);
    m_overlay->resetOverrides();
    m_overlay->setPosition(rect.location());
    m_overlay->setSize(rect.size());
    m_overlay->setDrawsContent(true);
    m_overlay->removeAnimation(s_fadeAnimationName);
    m_overlay->setOpacity(1.0);
    m_overlay->invalidate();

    // Animate overlay scale to indicate selection is started.
    if (isStartOfSelection) {
        WebAnimation shrinkAnimation = WebAnimation::shrinkAnimation(s_shrinkAnimationName, OverlayInitialScale, 1, OverlayShrinkAnimationDuration);
        m_overlay->addAnimation(shrinkAnimation);
    }
}

void DefaultTapHighlight::hide()
{
    if (!m_overlay)
        return;

    {
        MutexLocker lock(m_mutex);
        if (!m_visible)
            return;
        m_visible = false;
    }

    // Since WebAnimation is not thread safe, we create a new one each time instead of reusing the same object on different
    // threads (that would introduce race conditions).
    WebAnimation fadeAnimation = WebAnimation::fadeAnimation(s_fadeAnimationName, 1.0, 0.0, ActiveTextFadeAnimationDuration);

    // Normally, this method is called on the WebKit thread, but it can also be
    // called from the compositing thread.
    if (BlackBerry::Platform::webKitThreadMessageClient()->isCurrentThread())
        m_overlay->addAnimation(fadeAnimation);
    else if (BlackBerry::Platform::userInterfaceThreadMessageClient()->isCurrentThread())
        m_overlay->override()->addAnimation(fadeAnimation);
}

void DefaultTapHighlight::notifyFlushRequired(const GraphicsLayer* layer)
{
    m_page->notifyFlushRequired(layer);
}

void DefaultTapHighlight::paintContents(const GraphicsLayer*, GraphicsContext& c, GraphicsLayerPaintingPhase, const IntRect& /*inClip*/)
{
    if (!m_region.numRects())
        return;

    Path path(m_region.boundaryPath());

    c.save();
    const Platform::IntRect& rect = m_region.extents();
    c.translate(-rect.x(), -rect.y());

    // Draw tap highlight
    c.setFillColor(m_color, ColorSpaceDeviceRGB);
    c.fillPath(path);
    Color darker = Color(m_color.red(), m_color.green(), m_color.blue()); // Get rid of alpha.
    c.setStrokeColor(darker, ColorSpaceDeviceRGB);
    c.setStrokeThickness(1);
    c.strokePath(path);
    c.restore();
}

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)
