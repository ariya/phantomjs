/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#include "config.h"
#if USE(COORDINATED_GRAPHICS)

#include "WebPage.h"

#include "HitTestResult.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebPageProxyMessages.h"
#include <WebCore/EventHandler.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/ScrollView.h>

using namespace WebCore;

namespace WebKit {

#if ENABLE(TOUCH_ADJUSTMENT)
void WebPage::findZoomableAreaForPoint(const IntPoint& point, const IntSize& area)
{
    Node* node = 0;
    IntRect zoomableArea;
    bool foundAreaForTouchPoint = m_mainFrame->coreFrame()->eventHandler()->bestZoomableAreaForTouchPoint(point, IntSize(area.width() / 2, area.height() / 2), zoomableArea, node);

    if (!foundAreaForTouchPoint)
        return;

    ASSERT(node);

    if (node->document() && node->document()->view())
        zoomableArea = node->document()->view()->contentsToWindow(zoomableArea);

    send(Messages::WebPageProxy::DidFindZoomableArea(point, zoomableArea));
}

#else
void WebPage::findZoomableAreaForPoint(const IntPoint& point, const IntSize& area)
{
    UNUSED_PARAM(area);
    Frame* mainframe = m_mainFrame->coreFrame();
    HitTestResult result = mainframe->eventHandler()->hitTestResultAtPoint(mainframe->view()->windowToContents(point), HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);

    Node* node = result.innerNode();

    if (!node)
        return;

    IntRect zoomableArea = node->pixelSnappedBoundingBox();

    while (true) {
        bool found = !node->isTextNode() && !node->isShadowRoot();

        // No candidate found, bail out.
        if (!found && !node->parentNode())
            return;

        // Candidate found, and it is a better candidate than its parent.
        // NB: A parent is considered a better candidate iff the node is
        // contained by it and it is the only child.
        if (found && (!node->parentNode() || node->parentNode()->childNodeCount() != 1))
            break;

        node = node->parentNode();
        zoomableArea.unite(node->pixelSnappedBoundingBox());
    }

    if (node->document() && node->document()->frame() && node->document()->frame()->view()) {
        const ScrollView* view = node->document()->frame()->view();
        zoomableArea = view->contentsToWindow(zoomableArea);
    }

    send(Messages::WebPageProxy::DidFindZoomableArea(point, zoomableArea));
}
#endif

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)
