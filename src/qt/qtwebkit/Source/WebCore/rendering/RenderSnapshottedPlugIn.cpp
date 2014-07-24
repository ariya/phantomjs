/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "RenderSnapshottedPlugIn.h"

#include "CachedImage.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Cursor.h"
#include "Filter.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "Gradient.h"
#include "HTMLPlugInImageElement.h"
#include "ImageBuffer.h"
#include "MouseEvent.h"
#include "Page.h"
#include "PaintInfo.h"
#include "Path.h"
#include "PlatformMouseEvent.h"
#include "RenderView.h"
#include <wtf/StackStats.h>

namespace WebCore {

RenderSnapshottedPlugIn::RenderSnapshottedPlugIn(HTMLPlugInImageElement* element)
    : RenderEmbeddedObject(element)
    , m_snapshotResource(RenderImageResource::create())
    , m_isPotentialMouseActivation(false)
{
    m_snapshotResource->initialize(this);
}

RenderSnapshottedPlugIn::~RenderSnapshottedPlugIn()
{
    ASSERT(m_snapshotResource);
    m_snapshotResource->shutdown();
}

HTMLPlugInImageElement* RenderSnapshottedPlugIn::plugInImageElement() const
{
    return toHTMLPlugInImageElement(node());
}

void RenderSnapshottedPlugIn::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    LayoutSize oldSize = contentBoxRect().size();

    RenderEmbeddedObject::layout();

    LayoutSize newSize = contentBoxRect().size();
    if (newSize == oldSize)
        return;

    if (document()->view())
        document()->view()->addWidgetToUpdate(this);
}

void RenderSnapshottedPlugIn::updateSnapshot(PassRefPtr<Image> image)
{
    // Zero-size plugins will have no image.
    if (!image)
        return;

    m_snapshotResource->setCachedImage(new CachedImage(image.get()));
    repaint();
}

void RenderSnapshottedPlugIn::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.phase == PaintPhaseForeground && plugInImageElement()->displayState() < HTMLPlugInElement::Restarting) {
        paintSnapshot(paintInfo, paintOffset);
    }

    PaintPhase newPhase = (paintInfo.phase == PaintPhaseChildOutlines) ? PaintPhaseOutline : paintInfo.phase;
    newPhase = (newPhase == PaintPhaseChildBlockBackgrounds) ? PaintPhaseChildBlockBackground : newPhase;

    PaintInfo paintInfoForChild(paintInfo);
    paintInfoForChild.phase = newPhase;
    paintInfoForChild.updateSubtreePaintRootForChildren(this);

    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        LayoutPoint childPoint = flipForWritingModeForChild(child, paintOffset);
        if (!child->hasSelfPaintingLayer() && !child->isFloating())
            child->paint(paintInfoForChild, childPoint);
    }

    RenderEmbeddedObject::paint(paintInfo, paintOffset);
}

void RenderSnapshottedPlugIn::paintSnapshot(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    Image* image = m_snapshotResource->image().get();
    if (!image || image->isNull())
        return;

    LayoutUnit cWidth = contentWidth();
    LayoutUnit cHeight = contentHeight();
    if (!cWidth || !cHeight)
        return;

    GraphicsContext* context = paintInfo.context;
#if PLATFORM(MAC)
    if (style()->highlight() != nullAtom && !context->paintingDisabled())
        paintCustomHighlight(toPoint(paintOffset - location()), style()->highlight(), true);
#endif

    LayoutSize contentSize(cWidth, cHeight);
    LayoutPoint contentLocation = location() + paintOffset;
    contentLocation.move(borderLeft() + paddingLeft(), borderTop() + paddingTop());

    LayoutRect rect(contentLocation, contentSize);
    IntRect alignedRect = pixelSnappedIntRect(rect);
    if (alignedRect.width() <= 0 || alignedRect.height() <= 0)
        return;

    bool useLowQualityScaling = shouldPaintAtLowQuality(context, image, image, alignedRect.size());
    context->drawImage(image, style()->colorSpace(), alignedRect, CompositeSourceOver, shouldRespectImageOrientation(), useLowQualityScaling);
}

CursorDirective RenderSnapshottedPlugIn::getCursor(const LayoutPoint& point, Cursor& overrideCursor) const
{
    if (plugInImageElement()->displayState() < HTMLPlugInElement::Restarting) {
        overrideCursor = handCursor();
        return SetCursor;
    }
    return RenderEmbeddedObject::getCursor(point, overrideCursor);
}

void RenderSnapshottedPlugIn::handleEvent(Event* event)
{
    if (!event->isMouseEvent())
        return;

    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);

    // If we're a snapshotted plugin, we want to make sure we activate on
    // clicks even if the page is preventing our default behaviour. Otherwise
    // we can never restart. One we do restart, then the page will happily
    // block the new plugin in the normal renderer. All this means we have to
    // be on the lookout for a mouseup event that comes after a mousedown
    // event. The code below is not completely foolproof, but the worst that
    // could happen is that a snapshotted plugin restarts.

    if (event->type() == eventNames().mouseoutEvent)
        m_isPotentialMouseActivation = false;

    if (mouseEvent->button() != LeftButton)
        return;

    if (event->type() == eventNames().clickEvent || (m_isPotentialMouseActivation && event->type() == eventNames().mouseupEvent)) {
        m_isPotentialMouseActivation = false;
        bool clickWasOnOverlay = plugInImageElement()->partOfSnapshotOverlay(event->target()->toNode());
        plugInImageElement()->userDidClickSnapshot(mouseEvent, !clickWasOnOverlay);
        event->setDefaultHandled();
    } else if (event->type() == eventNames().mousedownEvent) {
        m_isPotentialMouseActivation = true;
        event->setDefaultHandled();
    }
}

} // namespace WebCore
