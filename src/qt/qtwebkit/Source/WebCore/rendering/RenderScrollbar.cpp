/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "RenderScrollbar.h"

#include "Frame.h"
#include "FrameView.h"
#include "RenderPart.h"
#include "RenderScrollbarPart.h"
#include "RenderScrollbarTheme.h"
#include "StyleInheritedData.h"
#include "StyleResolver.h"

namespace WebCore {

PassRefPtr<Scrollbar> RenderScrollbar::createCustomScrollbar(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, Node* ownerNode, Frame* owningFrame)
{
    return adoptRef(new RenderScrollbar(scrollableArea, orientation, ownerNode, owningFrame));
}

RenderScrollbar::RenderScrollbar(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, Node* ownerNode, Frame* owningFrame)
    : Scrollbar(scrollableArea, orientation, RegularScrollbar, RenderScrollbarTheme::renderScrollbarTheme())
    , m_owner(ownerNode)
    , m_owningFrame(owningFrame)
{
    ASSERT(ownerNode || owningFrame);

    // FIXME: We need to do this because RenderScrollbar::styleChanged is called as soon as the scrollbar is created.
    
    // Update the scrollbar size.
    int width = 0;
    int height = 0;
    updateScrollbarPart(ScrollbarBGPart);
    if (RenderScrollbarPart* part = m_parts.get(ScrollbarBGPart)) {
        part->layout();
        width = part->width();
        height = part->height();
    } else if (this->orientation() == HorizontalScrollbar)
        width = this->width();
    else
        height = this->height();

    setFrameRect(IntRect(0, 0, width, height));
}

RenderScrollbar::~RenderScrollbar()
{
    if (!m_parts.isEmpty()) {
        // When a scrollbar is detached from its parent (causing all parts removal) and 
        // ready to be destroyed, its destruction can be delayed because of RefPtr
        // maintained in other classes such as EventHandler (m_lastScrollbarUnderMouse).
        // Meanwhile, we can have a call to updateScrollbarPart which recreates the 
        // scrollbar part. So, we need to destroy these parts since we don't want them
        // to call on a destroyed scrollbar. See webkit bug 68009.
        updateScrollbarParts(true);
    }
}

RenderBox* RenderScrollbar::owningRenderer() const
{
    if (m_owningFrame) {
        RenderBox* currentRenderer = m_owningFrame->ownerRenderer();
        return currentRenderer;
    }
    return m_owner && m_owner->renderer() ? m_owner->renderer()->enclosingBox() : 0;
}

void RenderScrollbar::setParent(ScrollView* parent)
{
    Scrollbar::setParent(parent);
    if (!parent) {
        // Destroy all of the scrollbar's RenderBoxes.
        updateScrollbarParts(true);
    }
}

void RenderScrollbar::setEnabled(bool e)
{
    bool wasEnabled = enabled();
    Scrollbar::setEnabled(e);
    if (wasEnabled != e)
        updateScrollbarParts();
}

void RenderScrollbar::styleChanged()
{
    updateScrollbarParts();
}

void RenderScrollbar::paint(GraphicsContext* context, const IntRect& damageRect)
{
    if (context->updatingControlTints()) {
        updateScrollbarParts();
        return;
    }
    Scrollbar::paint(context, damageRect);
}

void RenderScrollbar::setHoveredPart(ScrollbarPart part)
{
    if (part == m_hoveredPart)
        return;

    ScrollbarPart oldPart = m_hoveredPart;
    m_hoveredPart = part;

    updateScrollbarPart(oldPart);
    updateScrollbarPart(m_hoveredPart);

    updateScrollbarPart(ScrollbarBGPart);
    updateScrollbarPart(TrackBGPart);
}

void RenderScrollbar::setPressedPart(ScrollbarPart part)
{
    ScrollbarPart oldPart = m_pressedPart;
    Scrollbar::setPressedPart(part);
    
    updateScrollbarPart(oldPart);
    updateScrollbarPart(part);
    
    updateScrollbarPart(ScrollbarBGPart);
    updateScrollbarPart(TrackBGPart);
}

PassRefPtr<RenderStyle> RenderScrollbar::getScrollbarPseudoStyle(ScrollbarPart partType, PseudoId pseudoId)
{
    if (!owningRenderer())
        return 0;

    RefPtr<RenderStyle> result = owningRenderer()->getUncachedPseudoStyle(PseudoStyleRequest(pseudoId, this, partType), owningRenderer()->style());
    // Scrollbars for root frames should always have background color 
    // unless explicitly specified as transparent. So we force it.
    // This is because WebKit assumes scrollbar to be always painted and missing background
    // causes visual artifact like non-repainted dirty region.
    if (result && m_owningFrame && m_owningFrame->view() && !m_owningFrame->view()->isTransparent() && !result->hasBackground())
        result->setBackgroundColor(Color::white);

    return result;
}

void RenderScrollbar::updateScrollbarParts(bool destroy)
{
    updateScrollbarPart(ScrollbarBGPart, destroy);
    updateScrollbarPart(BackButtonStartPart, destroy);
    updateScrollbarPart(ForwardButtonStartPart, destroy);
    updateScrollbarPart(BackTrackPart, destroy);
    updateScrollbarPart(ThumbPart, destroy);
    updateScrollbarPart(ForwardTrackPart, destroy);
    updateScrollbarPart(BackButtonEndPart, destroy);
    updateScrollbarPart(ForwardButtonEndPart, destroy);
    updateScrollbarPart(TrackBGPart, destroy);
    
    if (destroy)
        return;

    // See if the scrollbar's thickness changed.  If so, we need to mark our owning object as needing a layout.
    bool isHorizontal = orientation() == HorizontalScrollbar;    
    int oldThickness = isHorizontal ? height() : width();
    int newThickness = 0;
    RenderScrollbarPart* part = m_parts.get(ScrollbarBGPart);
    if (part) {
        part->layout();
        newThickness = isHorizontal ? part->height() : part->width();
    }
    
    if (newThickness != oldThickness) {
        setFrameRect(IntRect(location(), IntSize(isHorizontal ? width() : newThickness, isHorizontal ? newThickness : height())));
        if (RenderBox* box = owningRenderer())
            box->setChildNeedsLayout(true);
    }
}

static PseudoId pseudoForScrollbarPart(ScrollbarPart part)
{
    switch (part) {
        case BackButtonStartPart:
        case ForwardButtonStartPart:
        case BackButtonEndPart:
        case ForwardButtonEndPart:
            return SCROLLBAR_BUTTON;
        case BackTrackPart:
        case ForwardTrackPart:
            return SCROLLBAR_TRACK_PIECE;
        case ThumbPart:
            return SCROLLBAR_THUMB;
        case TrackBGPart:
            return SCROLLBAR_TRACK;
        case ScrollbarBGPart:
            return SCROLLBAR;
        case NoPart:
        case AllParts:
            break;
    }
    ASSERT_NOT_REACHED();
    return SCROLLBAR;
}

void RenderScrollbar::updateScrollbarPart(ScrollbarPart partType, bool destroy)
{
    if (partType == NoPart)
        return;

    RefPtr<RenderStyle> partStyle = !destroy ? getScrollbarPseudoStyle(partType,  pseudoForScrollbarPart(partType)) : PassRefPtr<RenderStyle>(0);
    
    bool needRenderer = !destroy && partStyle && partStyle->display() != NONE && partStyle->visibility() == VISIBLE;
    
    if (needRenderer && partStyle->display() != BLOCK) {
        // See if we are a button that should not be visible according to OS settings.
        ScrollbarButtonsPlacement buttonsPlacement = theme()->buttonsPlacement();
        switch (partType) {
            case BackButtonStartPart:
                needRenderer = (buttonsPlacement == ScrollbarButtonsSingle || buttonsPlacement == ScrollbarButtonsDoubleStart ||
                                buttonsPlacement == ScrollbarButtonsDoubleBoth);
                break;
            case ForwardButtonStartPart:
                needRenderer = (buttonsPlacement == ScrollbarButtonsDoubleStart || buttonsPlacement == ScrollbarButtonsDoubleBoth);
                break;
            case BackButtonEndPart:
                needRenderer = (buttonsPlacement == ScrollbarButtonsDoubleEnd || buttonsPlacement == ScrollbarButtonsDoubleBoth);
                break;
            case ForwardButtonEndPart:
                needRenderer = (buttonsPlacement == ScrollbarButtonsSingle || buttonsPlacement == ScrollbarButtonsDoubleEnd ||
                                buttonsPlacement == ScrollbarButtonsDoubleBoth);
                break;
            default:
                break;
        }
    }
    
    RenderScrollbarPart* partRenderer = m_parts.get(partType);
    if (!partRenderer && needRenderer) {
        partRenderer = RenderScrollbarPart::createAnonymous(owningRenderer()->document(), this, partType);
        m_parts.set(partType, partRenderer);
    } else if (partRenderer && !needRenderer) {
        m_parts.remove(partType);
        partRenderer->destroy();
        partRenderer = 0;
    }
    
    if (partRenderer)
        partRenderer->setStyle(partStyle.release());
}

void RenderScrollbar::paintPart(GraphicsContext* graphicsContext, ScrollbarPart partType, const IntRect& rect)
{
    RenderScrollbarPart* partRenderer = m_parts.get(partType);
    if (!partRenderer)
        return;
    partRenderer->paintIntoRect(graphicsContext, location(), rect);
}

IntRect RenderScrollbar::buttonRect(ScrollbarPart partType)
{
    RenderScrollbarPart* partRenderer = m_parts.get(partType);
    if (!partRenderer)
        return IntRect();
        
    partRenderer->layout();
    
    bool isHorizontal = orientation() == HorizontalScrollbar;
    if (partType == BackButtonStartPart)
        return IntRect(location(), IntSize(isHorizontal ? partRenderer->pixelSnappedWidth() : width(), isHorizontal ? height() : partRenderer->pixelSnappedHeight()));
    if (partType == ForwardButtonEndPart)
        return IntRect(isHorizontal ? x() + width() - partRenderer->pixelSnappedWidth() : x(),
                       isHorizontal ? y() : y() + height() - partRenderer->pixelSnappedHeight(),
                       isHorizontal ? partRenderer->pixelSnappedWidth() : width(),
                       isHorizontal ? height() : partRenderer->pixelSnappedHeight());
    
    if (partType == ForwardButtonStartPart) {
        IntRect previousButton = buttonRect(BackButtonStartPart);
        return IntRect(isHorizontal ? x() + previousButton.width() : x(),
                       isHorizontal ? y() : y() + previousButton.height(),
                       isHorizontal ? partRenderer->pixelSnappedWidth() : width(),
                       isHorizontal ? height() : partRenderer->pixelSnappedHeight());
    }
    
    IntRect followingButton = buttonRect(ForwardButtonEndPart);
    return IntRect(isHorizontal ? x() + width() - followingButton.width() - partRenderer->pixelSnappedWidth() : x(),
                   isHorizontal ? y() : y() + height() - followingButton.height() - partRenderer->pixelSnappedHeight(),
                   isHorizontal ? partRenderer->pixelSnappedWidth() : width(),
                   isHorizontal ? height() : partRenderer->pixelSnappedHeight());
}

IntRect RenderScrollbar::trackRect(int startLength, int endLength)
{
    RenderScrollbarPart* part = m_parts.get(TrackBGPart);
    if (part)
        part->layout();

    if (orientation() == HorizontalScrollbar) {
        int marginLeft = part ? static_cast<int>(part->marginLeft()) : 0;
        int marginRight = part ? static_cast<int>(part->marginRight()) : 0;
        startLength += marginLeft;
        endLength += marginRight;
        int totalLength = startLength + endLength;
        return IntRect(x() + startLength, y(), width() - totalLength, height());
    }
    
    int marginTop = part ? static_cast<int>(part->marginTop()) : 0;
    int marginBottom = part ? static_cast<int>(part->marginBottom()) : 0;
    startLength += marginTop;
    endLength += marginBottom;
    int totalLength = startLength + endLength;

    return IntRect(x(), y() + startLength, width(), height() - totalLength);
}

IntRect RenderScrollbar::trackPieceRectWithMargins(ScrollbarPart partType, const IntRect& oldRect)
{
    RenderScrollbarPart* partRenderer = m_parts.get(partType);
    if (!partRenderer)
        return oldRect;
    
    partRenderer->layout();
    
    IntRect rect = oldRect;
    if (orientation() == HorizontalScrollbar) {
        rect.setX(rect.x() + partRenderer->marginLeft());
        rect.setWidth(rect.width() - partRenderer->marginWidth());
    } else {
        rect.setY(rect.y() + partRenderer->marginTop());
        rect.setHeight(rect.height() - partRenderer->marginHeight());
    }
    return rect;
}

int RenderScrollbar::minimumThumbLength()
{
    RenderScrollbarPart* partRenderer = m_parts.get(ThumbPart);
    if (!partRenderer)
        return 0;    
    partRenderer->layout();
    return orientation() == HorizontalScrollbar ? partRenderer->width() : partRenderer->height();
}

}
