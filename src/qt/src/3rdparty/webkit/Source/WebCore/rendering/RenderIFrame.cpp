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

#include "config.h"
#include "RenderIFrame.h"

#include "FrameView.h"
#include "HTMLNames.h"
#include "HTMLIFrameElement.h"
#include "RenderView.h"
#include "Settings.h"

namespace WebCore {

using namespace HTMLNames;
    
RenderIFrame::RenderIFrame(Element* element)
    : RenderFrameBase(element)
{
}

void RenderIFrame::computeLogicalHeight()
{
    RenderPart::computeLogicalHeight();
    if (!flattenFrame())
         return;

    HTMLIFrameElement* frame = static_cast<HTMLIFrameElement*>(node());
    bool isScrollable = frame->scrollingMode() != ScrollbarAlwaysOff;

    if (isScrollable || !style()->height().isFixed()) {
        FrameView* view = static_cast<FrameView*>(widget());
        if (!view)
            return;
        int border = borderTop() + borderBottom();
        setHeight(max(height(), view->contentsHeight() + border));
    }
}

void RenderIFrame::computeLogicalWidth()
{
    RenderPart::computeLogicalWidth();
    if (!flattenFrame())
        return;

    HTMLIFrameElement* frame = static_cast<HTMLIFrameElement*>(node());
    bool isScrollable = frame->scrollingMode() != ScrollbarAlwaysOff;

    if (isScrollable || !style()->width().isFixed()) {
        FrameView* view = static_cast<FrameView*>(widget());
        if (!view)
            return;
        int border = borderLeft() + borderRight();
        setWidth(max(width(), view->contentsWidth() + border));
    }
}

bool RenderIFrame::flattenFrame()
{
    if (!node() || !node()->hasTagName(iframeTag))
        return false;

    HTMLIFrameElement* element = static_cast<HTMLIFrameElement*>(node());
    bool isScrollable = element->scrollingMode() != ScrollbarAlwaysOff;

    if (style()->width().isFixed() && style()->height().isFixed()) {
        if (!isScrollable)
            return false;
        if (style()->width().value() <= 0 || style()->height().value() <= 0)
            return false;
    }

    Frame* frame = element->document()->frame();
    bool enabled = frame && frame->settings()->frameFlatteningEnabled();

    if (!enabled || !frame->page())
        return false;

    FrameView* view = frame->page()->mainFrame()->view();
    if (!view)
        return false;

    // Do not flatten offscreen inner frames during frame flattening, as flattening might make them visible.
    IntRect boundingRect = absoluteBoundingBoxRect();
    return boundingRect.maxX() > 0 && boundingRect.maxY() > 0;
}

void RenderIFrame::layout()
{
    ASSERT(needsLayout());

    RenderPart::computeLogicalWidth();
    RenderPart::computeLogicalHeight();

    if (flattenFrame()) {
        layoutWithFlattening(style()->width().isFixed(), style()->height().isFixed());
        return;
    }

    RenderPart::layout();

    m_overflow.clear();
    addShadowOverflow();
    updateLayerTransform();

    setNeedsLayout(false);
}

}
