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

#include "config.h"
#include "TapHighlightController.h"

#if ENABLE(TOUCH_EVENTS)

#include "ShareableBitmap.h"
#include "WKPage.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/GestureTapHighlighter.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/Page.h>

#include <WebCore/RenderObject.h>

using namespace std;
using namespace WebCore;

namespace WebKit {

TapHighlightController::TapHighlightController(WebPage* webPage)
    : m_webPage(webPage)
    , m_overlay(0)
{
}

TapHighlightController::~TapHighlightController()
{
}

void TapHighlightController::highlight(Node* node)
{
    ASSERT(node);

    m_path = GestureTapHighlighter::pathForNodeHighlight(node);
    m_color = node->renderer()->style()->tapHighlightColor();

    if (!m_overlay) {
        RefPtr<PageOverlay> overlay = PageOverlay::create(this);
        m_overlay = overlay.get();
        m_webPage->installPageOverlay(overlay.release());
    } else
        m_overlay->setNeedsDisplay();
}

void TapHighlightController::hideHighlight()
{
    if (m_overlay)
        m_webPage->uninstallPageOverlay(m_overlay, /* fadeout */ true);
}

void TapHighlightController::pageOverlayDestroyed(PageOverlay*)
{
}

void TapHighlightController::willMoveToWebPage(PageOverlay*, WebPage* webPage)
{
    if (webPage)
        return;

    // The page overlay is moving away from the web page, reset it.
    ASSERT(m_overlay);
    m_overlay = 0;
}

void TapHighlightController::didMoveToWebPage(PageOverlay*, WebPage*)
{
}

static Color highlightColor(Color baseColor, float fractionFadedIn)
{
    return Color(baseColor.red(), baseColor.green(), baseColor.blue(), int(baseColor.alpha() * fractionFadedIn));
}

void TapHighlightController::drawRect(PageOverlay* pageOverlay, GraphicsContext& context, const IntRect& /*dirtyRect*/)
{
    if (m_path.isEmpty())
        return;

    {
        GraphicsContextStateSaver stateSaver(context);
        if (m_webPage->drawingArea()->pageOverlayShouldApplyFadeWhenPainting())
            context.setFillColor(highlightColor(m_color, pageOverlay->fractionFadedIn() * 0.5f), ColorSpaceSRGB);
        else
            context.setFillColor(highlightColor(m_color, 0.5f), ColorSpaceSRGB);
        context.fillPath(m_path);
    }
}

bool TapHighlightController::mouseEvent(PageOverlay*, const WebMouseEvent&)
{
    return false;
}

} // namespace WebKit

#endif // ENABLE(TOUCH_EVENTS)
