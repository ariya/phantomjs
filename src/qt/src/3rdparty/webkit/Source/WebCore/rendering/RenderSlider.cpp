/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "RenderSlider.h"

#include "CSSPropertyNames.h"
#include "CSSStyleSelector.h"
#include "Document.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "MediaControlElements.h"
#include "MouseEvent.h"
#include "Node.h"
#include "RenderLayer.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "ShadowRoot.h"
#include "SliderThumbElement.h"
#include "StepRange.h"
#include <wtf/MathExtras.h>

using std::min;

namespace WebCore {

static const int defaultTrackLength = 129;

// Returns a value between 0 and 1.
static double sliderPosition(HTMLInputElement* element)
{
    StepRange range(element);
    return range.proportionFromValue(range.valueFromElement(element));
}

RenderSlider::RenderSlider(HTMLInputElement* element)
    : RenderBlock(element)
{
}

RenderSlider::~RenderSlider()
{
}

int RenderSlider::baselinePosition(FontBaseline, bool /*firstLine*/, LineDirectionMode, LinePositionMode) const
{
    // FIXME: Patch this function for writing-mode.
    return height() + marginTop();
}

void RenderSlider::computePreferredLogicalWidths()
{
    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    if (style()->width().isFixed() && style()->width().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = computeContentBoxLogicalWidth(style()->width().value());
    else
        m_maxPreferredLogicalWidth = defaultTrackLength * style()->effectiveZoom();

    if (style()->minWidth().isFixed() && style()->minWidth().value() > 0) {
        m_maxPreferredLogicalWidth = max(m_maxPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->minWidth().value()));
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->minWidth().value()));
    } else if (style()->width().isPercent() || (style()->width().isAuto() && style()->height().isPercent()))
        m_minPreferredLogicalWidth = 0;
    else
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth;
    
    if (style()->maxWidth().isFixed() && style()->maxWidth().value() != undefinedLength) {
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->maxWidth().value()));
        m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->maxWidth().value()));
    }

    int toAdd = borderAndPaddingWidth();
    m_minPreferredLogicalWidth += toAdd;
    m_maxPreferredLogicalWidth += toAdd;

    setPreferredLogicalWidthsDirty(false); 
}

IntRect RenderSlider::thumbRect()
{
    SliderThumbElement* thumbElement = shadowSliderThumb();
    if (!thumbElement)
        return IntRect();

    IntRect thumbRect;
    RenderBox* thumb = toRenderBox(thumbElement->renderer());

    thumbRect.setWidth(thumb->style()->width().calcMinValue(contentWidth()));
    thumbRect.setHeight(thumb->style()->height().calcMinValue(contentHeight()));

    double fraction = sliderPosition(static_cast<HTMLInputElement*>(node()));
    IntRect contentRect = contentBoxRect();
    if (style()->appearance() == SliderVerticalPart || style()->appearance() == MediaVolumeSliderPart) {
        thumbRect.setX(contentRect.x() + (contentRect.width() - thumbRect.width()) / 2);
        thumbRect.setY(contentRect.y() + static_cast<int>(nextafter((contentRect.height() - thumbRect.height()) + 1, 0) * (1 - fraction)));
    } else {
        thumbRect.setX(contentRect.x() + static_cast<int>(nextafter((contentRect.width() - thumbRect.width()) + 1, 0) * fraction));
        thumbRect.setY(contentRect.y() + (contentRect.height() - thumbRect.height()) / 2);
    }

    return thumbRect;
}

void RenderSlider::layout()
{
    ASSERT(needsLayout());

    SliderThumbElement* thumbElement = shadowSliderThumb();
    RenderBox* thumb = thumbElement ? toRenderBox(thumbElement->renderer()) : 0;

    IntSize baseSize(borderAndPaddingWidth(), borderAndPaddingHeight());

    if (thumb) {
        // Allow the theme to set the size of the thumb.
        if (thumb->style()->hasAppearance()) {
            // FIXME: This should pass the style, not the renderer, to the theme.
            theme()->adjustSliderThumbSize(thumb);
        }

        baseSize.expand(thumb->style()->width().calcMinValue(0), thumb->style()->height().calcMinValue(0));
    }

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());

    IntSize oldSize = size();

    setSize(baseSize);
    computeLogicalWidth();
    computeLogicalHeight();
    updateLayerTransform();

    m_overflow.clear();

    if (thumb) {
        if (oldSize != size())
            thumb->setChildNeedsLayout(true, false);

        LayoutStateMaintainer statePusher(view(), this, IntSize(x(), y()), style()->isFlippedBlocksWritingMode());

        IntRect oldThumbRect = thumb->frameRect();

        thumb->layoutIfNeeded();

        IntRect rect = thumbRect();
        thumb->setFrameRect(rect);
        if (thumb->checkForRepaintDuringLayout())
            thumb->repaintDuringLayoutIfMoved(oldThumbRect);

        statePusher.pop();
        addOverflowFromChild(thumb);
    }

    repainter.repaintAfterLayout();    

    setNeedsLayout(false);
}

SliderThumbElement* RenderSlider::shadowSliderThumb() const
{
    Node* shadow = static_cast<Element*>(node())->shadowRoot();
    return shadow ? toSliderThumbElement(shadow->firstChild()) : 0;
}

bool RenderSlider::inDragMode() const
{
    SliderThumbElement* thumbElement = shadowSliderThumb();
    return thumbElement && thumbElement->inDragMode();
}

} // namespace WebCore
