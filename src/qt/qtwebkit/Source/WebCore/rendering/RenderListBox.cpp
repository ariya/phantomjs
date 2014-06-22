/*
 * Copyright (C) 2006, 2007, 2008, 2011 Apple Inc. All rights reserved.
 *               2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RenderListBox.h"

#include "AXObjectCache.h"
#include "CSSFontSelector.h"
#include "Document.h"
#include "DocumentEventQueue.h"
#include "EventHandler.h"
#include "FocusController.h"
#include "FontCache.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLOptGroupElement.h"
#include "HTMLSelectElement.h"
#include "HitTestResult.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderLayer.h"
#include "RenderScrollbar.h"
#include "RenderText.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "Scrollbar.h"
#include "ScrollbarTheme.h"
#include "SpatialNavigation.h"
#include "StyleResolver.h"
#include <math.h>
#include <wtf/StackStats.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;
 
const int rowSpacing = 1;

const int optionsSpacingHorizontal = 2;

// The minSize constant was originally defined to render scrollbars correctly.
// This might vary for different platforms.
const int minSize = 4;

// Default size when the multiple attribute is present but size attribute is absent.
const int defaultSize = 4;

// FIXME: This hardcoded baselineAdjustment is what we used to do for the old
// widget, but I'm not sure this is right for the new control.
const int baselineAdjustment = 7;

RenderListBox::RenderListBox(Element* element)
    : RenderBlock(element)
    , m_optionsChanged(true)
    , m_scrollToRevealSelectionAfterLayout(false)
    , m_inAutoscroll(false)
    , m_optionsWidth(0)
    , m_indexOffset(0)
{
    ASSERT(element);
    ASSERT(element->isHTMLElement());
    ASSERT(element->hasTagName(HTMLNames::selectTag));

    if (FrameView* frameView = frame()->view())
        frameView->addScrollableArea(this);
}

RenderListBox::~RenderListBox()
{
    setHasVerticalScrollbar(false);

    if (FrameView* frameView = frame()->view())
        frameView->removeScrollableArea(this);
}

inline HTMLSelectElement* RenderListBox::selectElement() const
{
    return toHTMLSelectElement(node());
}

void RenderListBox::updateFromElement()
{
    FontCachePurgePreventer fontCachePurgePreventer;

    if (m_optionsChanged) {
        const Vector<HTMLElement*>& listItems = selectElement()->listItems();
        int size = numItems();
        
        float width = 0;
        for (int i = 0; i < size; ++i) {
            HTMLElement* element = listItems[i];
            String text;
            Font itemFont = style()->font();
            if (isHTMLOptionElement(element))
                text = toHTMLOptionElement(element)->textIndentedToRespectGroupLabel();
            else if (isHTMLOptGroupElement(element)) {
                text = toHTMLOptGroupElement(element)->groupLabelText();
                FontDescription d = itemFont.fontDescription();
                d.setWeight(d.bolderWeight());
                itemFont = Font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
                itemFont.update(document()->ensureStyleResolver()->fontSelector());
            }

            if (!text.isEmpty()) {
                applyTextTransform(style(), text, ' ');
                // FIXME: Why is this always LTR? Can't text direction affect the width?
                TextRun textRun = constructTextRun(this, itemFont, text, style(), TextRun::AllowTrailingExpansion);
                textRun.disableRoundingHacks();
                float textWidth = itemFont.width(textRun);
                width = max(width, textWidth);
            }
        }
        m_optionsWidth = static_cast<int>(ceilf(width));
        m_optionsChanged = false;
        
        setHasVerticalScrollbar(true);

        setNeedsLayoutAndPrefWidthsRecalc();
    }
}

bool RenderListBox::canBeReplacedWithInlineRunIn() const
{
    return false;
}

void RenderListBox::selectionChanged()
{
    repaint();
    if (!m_inAutoscroll) {
        if (m_optionsChanged || needsLayout())
            m_scrollToRevealSelectionAfterLayout = true;
        else
            scrollToRevealSelection();
    }
    
    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->selectedChildrenChanged(this);
}

void RenderListBox::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    RenderBlock::layout();

    if (m_vBar) {
        bool enabled = numVisibleItems() < numItems();
        m_vBar->setEnabled(enabled);
        m_vBar->setSteps(1, max(1, numVisibleItems() - 1), itemHeight());
        m_vBar->setProportion(numVisibleItems(), numItems());
        if (!enabled) {
            scrollToOffsetWithoutAnimation(VerticalScrollbar, 0);
            m_indexOffset = 0;
        }
    }

    if (m_scrollToRevealSelectionAfterLayout) {
        LayoutStateDisabler layoutStateDisabler(view());
        scrollToRevealSelection();
    }
}

void RenderListBox::scrollToRevealSelection()
{    
    HTMLSelectElement* select = selectElement();

    m_scrollToRevealSelectionAfterLayout = false;

    int firstIndex = select->activeSelectionStartListIndex();
    if (firstIndex >= 0 && !listIndexIsVisible(select->activeSelectionEndListIndex()))
        scrollToRevealElementAtListIndex(firstIndex);
}

void RenderListBox::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    maxLogicalWidth = m_optionsWidth + 2 * optionsSpacingHorizontal;
    if (m_vBar)
        maxLogicalWidth += m_vBar->width();
    if (!style()->width().isPercent())
        minLogicalWidth = maxLogicalWidth;
}

void RenderListBox::computePreferredLogicalWidths()
{
    ASSERT(!m_optionsChanged);

    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    if (style()->width().isFixed() && style()->width().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = adjustContentBoxLogicalWidthForBoxSizing(style()->width().value());
    else
        computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    if (style()->minWidth().isFixed() && style()->minWidth().value() > 0) {
        m_maxPreferredLogicalWidth = max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->minWidth().value()));
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->minWidth().value()));
    }

    if (style()->maxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->maxWidth().value()));
        m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->maxWidth().value()));
    }

    LayoutUnit toAdd = borderAndPaddingWidth();
    m_minPreferredLogicalWidth += toAdd;
    m_maxPreferredLogicalWidth += toAdd;
                                
    setPreferredLogicalWidthsDirty(false);
}

int RenderListBox::size() const
{
    int specifiedSize = selectElement()->size();
    if (specifiedSize > 1)
        return max(minSize, specifiedSize);

    return defaultSize;
}

int RenderListBox::numVisibleItems() const
{
    // Only count fully visible rows. But don't return 0 even if only part of a row shows.
    return max<int>(1, (contentHeight() + rowSpacing) / itemHeight());
}

int RenderListBox::numItems() const
{
    return selectElement()->listItems().size();
}

LayoutUnit RenderListBox::listHeight() const
{
    return itemHeight() * numItems() - rowSpacing;
}

void RenderListBox::computeLogicalHeight(LayoutUnit, LayoutUnit logicalTop, LogicalExtentComputedValues& computedValues) const
{
    LayoutUnit height = itemHeight() * size() - rowSpacing + borderAndPaddingHeight();
    RenderBox::computeLogicalHeight(height, logicalTop, computedValues);
}

int RenderListBox::baselinePosition(FontBaseline baselineType, bool firstLine, LineDirectionMode lineDirection, LinePositionMode linePositionMode) const
{
    return RenderBox::baselinePosition(baselineType, firstLine, lineDirection, linePositionMode) - baselineAdjustment;
}

LayoutRect RenderListBox::itemBoundingBoxRect(const LayoutPoint& additionalOffset, int index)
{
    return LayoutRect(additionalOffset.x() + borderLeft() + paddingLeft(),
                   additionalOffset.y() + borderTop() + paddingTop() + itemHeight() * (index - m_indexOffset),
                   contentWidth(), itemHeight());
}
    
void RenderListBox::paintObject(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (style()->visibility() != VISIBLE)
        return;
    
    int listItemsSize = numItems();

    if (paintInfo.phase == PaintPhaseForeground) {
        int index = m_indexOffset;
        while (index < listItemsSize && index <= m_indexOffset + numVisibleItems()) {
            paintItemForeground(paintInfo, paintOffset, index);
            index++;
        }
    }

    // Paint the children.
    RenderBlock::paintObject(paintInfo, paintOffset);

    switch (paintInfo.phase) {
    // Depending on whether we have overlay scrollbars they
    // get rendered in the foreground or background phases
    case PaintPhaseForeground:
        if (m_vBar->isOverlayScrollbar())
            paintScrollbar(paintInfo, paintOffset);
        break;
    case PaintPhaseBlockBackground:
        if (!m_vBar->isOverlayScrollbar())
            paintScrollbar(paintInfo, paintOffset);
        break;
    case PaintPhaseChildBlockBackground:
    case PaintPhaseChildBlockBackgrounds: {
        int index = m_indexOffset;
        while (index < listItemsSize && index <= m_indexOffset + numVisibleItems()) {
            paintItemBackground(paintInfo, paintOffset, index);
            index++;
        }
        break;
    }
    default:
        break;
    }
}

void RenderListBox::addFocusRingRects(Vector<IntRect>& rects, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer)
{
    if (!isSpatialNavigationEnabled(frame()))
        return RenderBlock::addFocusRingRects(rects, additionalOffset, paintContainer);

    HTMLSelectElement* select = selectElement();

    // Focus the last selected item.
    int selectedItem = select->activeSelectionEndListIndex();
    if (selectedItem >= 0) {
        rects.append(pixelSnappedIntRect(itemBoundingBoxRect(additionalOffset, selectedItem)));
        return;
    }

    // No selected items, find the first non-disabled item.
    int size = numItems();
    const Vector<HTMLElement*>& listItems = select->listItems();
    for (int i = 0; i < size; ++i) {
        HTMLElement* element = listItems[i];
        if (isHTMLOptionElement(element) && !element->isDisabledFormControl()) {
            select->setActiveSelectionEndIndex(i);
            rects.append(pixelSnappedIntRect(itemBoundingBoxRect(additionalOffset, i)));
            return;
        }
    }
}

void RenderListBox::paintScrollbar(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (m_vBar) {
        IntRect scrollRect = pixelSnappedIntRect(paintOffset.x() + width() - borderRight() - m_vBar->width(),
            paintOffset.y() + borderTop(),
            m_vBar->width(),
            height() - (borderTop() + borderBottom()));
        m_vBar->setFrameRect(scrollRect);
        m_vBar->paint(paintInfo.context, paintInfo.rect);
    }
}

static LayoutSize itemOffsetForAlignment(TextRun textRun, RenderStyle* itemStyle, Font itemFont, LayoutRect itemBoudingBox)
{
    ETextAlign actualAlignment = itemStyle->textAlign();
    // FIXME: Firefox doesn't respect JUSTIFY. Should we?
    // FIXME: Handle TAEND here
    if (actualAlignment == TASTART || actualAlignment == JUSTIFY)
      actualAlignment = itemStyle->isLeftToRightDirection() ? LEFT : RIGHT;

    LayoutSize offset = LayoutSize(0, itemFont.fontMetrics().ascent());
    if (actualAlignment == RIGHT || actualAlignment == WEBKIT_RIGHT) {
        float textWidth = itemFont.width(textRun);
        offset.setWidth(itemBoudingBox.width() - textWidth - optionsSpacingHorizontal);
    } else if (actualAlignment == CENTER || actualAlignment == WEBKIT_CENTER) {
        float textWidth = itemFont.width(textRun);
        offset.setWidth((itemBoudingBox.width() - textWidth) / 2);
    } else
        offset.setWidth(optionsSpacingHorizontal);
    return offset;
}

void RenderListBox::paintItemForeground(PaintInfo& paintInfo, const LayoutPoint& paintOffset, int listIndex)
{
    FontCachePurgePreventer fontCachePurgePreventer;

    HTMLSelectElement* select = selectElement();

    const Vector<HTMLElement*>& listItems = select->listItems();
    HTMLElement* element = listItems[listIndex];

    RenderStyle* itemStyle = element->renderStyle();
    if (!itemStyle)
        itemStyle = style();

    if (itemStyle->visibility() == HIDDEN)
        return;

    String itemText;
    bool isOptionElement = isHTMLOptionElement(element);
    if (isOptionElement)
        itemText = toHTMLOptionElement(element)->textIndentedToRespectGroupLabel();
    else if (isHTMLOptGroupElement(element))
        itemText = toHTMLOptGroupElement(element)->groupLabelText();
    applyTextTransform(style(), itemText, ' ');

    Color textColor = element->renderStyle() ? element->renderStyle()->visitedDependentColor(CSSPropertyColor) : style()->visitedDependentColor(CSSPropertyColor);
    if (isOptionElement && toHTMLOptionElement(element)->selected()) {
        if (frame()->selection()->isFocusedAndActive() && document()->focusedElement() == node())
            textColor = theme()->activeListBoxSelectionForegroundColor();
        // Honor the foreground color for disabled items
        else if (!element->isDisabledFormControl() && !select->isDisabledFormControl())
            textColor = theme()->inactiveListBoxSelectionForegroundColor();
    }

    ColorSpace colorSpace = itemStyle->colorSpace();
    paintInfo.context->setFillColor(textColor, colorSpace);

    TextRun textRun(itemText, 0, 0, TextRun::AllowTrailingExpansion, itemStyle->direction(), isOverride(itemStyle->unicodeBidi()), true, TextRun::NoRounding);
    Font itemFont = style()->font();
    LayoutRect r = itemBoundingBoxRect(paintOffset, listIndex);
    r.move(itemOffsetForAlignment(textRun, itemStyle, itemFont, r));

    if (isHTMLOptGroupElement(element)) {
        FontDescription d = itemFont.fontDescription();
        d.setWeight(d.bolderWeight());
        itemFont = Font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
        itemFont.update(document()->ensureStyleResolver()->fontSelector());
    }

    // Draw the item text
    paintInfo.context->drawBidiText(itemFont, textRun, roundedIntPoint(r.location()));
}

void RenderListBox::paintItemBackground(PaintInfo& paintInfo, const LayoutPoint& paintOffset, int listIndex)
{
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    HTMLElement* element = listItems[listIndex];

    Color backColor;
    if (isHTMLOptionElement(element) && toHTMLOptionElement(element)->selected()) {
        if (frame()->selection()->isFocusedAndActive() && document()->focusedElement() == node())
            backColor = theme()->activeListBoxSelectionBackgroundColor();
        else
            backColor = theme()->inactiveListBoxSelectionBackgroundColor();
    } else
        backColor = element->renderStyle() ? element->renderStyle()->visitedDependentColor(CSSPropertyBackgroundColor) : style()->visitedDependentColor(CSSPropertyBackgroundColor);

    // Draw the background for this list box item
    if (!element->renderStyle() || element->renderStyle()->visibility() != HIDDEN) {
        ColorSpace colorSpace = element->renderStyle() ? element->renderStyle()->colorSpace() : style()->colorSpace();
        LayoutRect itemRect = itemBoundingBoxRect(paintOffset, listIndex);
        itemRect.intersect(controlClipRect(paintOffset));
        paintInfo.context->fillRect(pixelSnappedIntRect(itemRect), backColor, colorSpace);
    }
}

bool RenderListBox::isPointInOverflowControl(HitTestResult& result, const LayoutPoint& locationInContainer, const LayoutPoint& accumulatedOffset)
{
    if (!m_vBar || !m_vBar->shouldParticipateInHitTesting())
        return false;

    LayoutRect vertRect(accumulatedOffset.x() + width() - borderRight() - m_vBar->width(),
                        accumulatedOffset.y() + borderTop(),
                        m_vBar->width(),
                        height() - borderTop() - borderBottom());

    if (vertRect.contains(locationInContainer)) {
        result.setScrollbar(m_vBar.get());
        return true;
    }
    return false;
}

int RenderListBox::listIndexAtOffset(const LayoutSize& offset)
{
    if (!numItems())
        return -1;

    if (offset.height() < borderTop() + paddingTop() || offset.height() > height() - paddingBottom() - borderBottom())
        return -1;

    int scrollbarWidth = m_vBar ? m_vBar->width() : 0;
    if (offset.width() < borderLeft() + paddingLeft() || offset.width() > width() - borderRight() - paddingRight() - scrollbarWidth)
        return -1;

    int newOffset = (offset.height() - borderTop() - paddingTop()) / itemHeight() + m_indexOffset;
    return newOffset < numItems() ? newOffset : -1;
}

void RenderListBox::panScroll(const IntPoint& panStartMousePosition)
{
    const int maxSpeed = 20;
    const int iconRadius = 7;
    const int speedReducer = 4;

    // FIXME: This doesn't work correctly with transforms.
    FloatPoint absOffset = localToAbsolute();

    IntPoint lastKnownMousePosition = frame()->eventHandler()->lastKnownMousePosition();
    // We need to check if the last known mouse position is out of the window. When the mouse is out of the window, the position is incoherent
    static IntPoint previousMousePosition;
    if (lastKnownMousePosition.y() < 0)
        lastKnownMousePosition = previousMousePosition;
    else
        previousMousePosition = lastKnownMousePosition;

    int yDelta = lastKnownMousePosition.y() - panStartMousePosition.y();

    // If the point is too far from the center we limit the speed
    yDelta = max<int>(min<int>(yDelta, maxSpeed), -maxSpeed);
    
    if (abs(yDelta) < iconRadius) // at the center we let the space for the icon
        return;

    if (yDelta > 0)
        //offsetY = view()->viewHeight();
        absOffset.move(0, listHeight());
    else if (yDelta < 0)
        yDelta--;

    // Let's attenuate the speed
    yDelta /= speedReducer;

    IntPoint scrollPoint(0, 0);
    scrollPoint.setY(absOffset.y() + yDelta);
    int newOffset = scrollToward(scrollPoint);
    if (newOffset < 0) 
        return;

    m_inAutoscroll = true;
    HTMLSelectElement* select = selectElement();
    select->updateListBoxSelection(!select->multiple());
    m_inAutoscroll = false;
}

int RenderListBox::scrollToward(const IntPoint& destination)
{
    // FIXME: This doesn't work correctly with transforms.
    FloatPoint absPos = localToAbsolute();
    IntSize positionOffset = roundedIntSize(destination - absPos);

    int rows = numVisibleItems();
    int offset = m_indexOffset;
    
    if (positionOffset.height() < borderTop() + paddingTop() && scrollToRevealElementAtListIndex(offset - 1))
        return offset - 1;
    
    if (positionOffset.height() > height() - paddingBottom() - borderBottom() && scrollToRevealElementAtListIndex(offset + rows))
        return offset + rows - 1;
    
    return listIndexAtOffset(positionOffset);
}

void RenderListBox::autoscroll(const IntPoint&)
{
    IntPoint pos = frame()->view()->windowToContents(frame()->eventHandler()->lastKnownMousePosition());

    int endIndex = scrollToward(pos);
    if (selectElement()->isDisabledFormControl())
        return;

    if (endIndex >= 0) {
        HTMLSelectElement* select = selectElement();
        m_inAutoscroll = true;

        if (!select->multiple())
            select->setActiveSelectionAnchorIndex(endIndex);

        select->setActiveSelectionEndIndex(endIndex);
        select->updateListBoxSelection(!select->multiple());
        m_inAutoscroll = false;
    }
}

void RenderListBox::stopAutoscroll()
{
    if (selectElement()->isDisabledFormControl())
        return;

    selectElement()->listBoxOnChange();
}

bool RenderListBox::scrollToRevealElementAtListIndex(int index)
{
    if (index < 0 || index >= numItems() || listIndexIsVisible(index))
        return false;

    int newOffset;
    if (index < m_indexOffset)
        newOffset = index;
    else
        newOffset = index - numVisibleItems() + 1;

    scrollToOffsetWithoutAnimation(VerticalScrollbar, newOffset);

    return true;
}

bool RenderListBox::listIndexIsVisible(int index)
{    
    return index >= m_indexOffset && index < m_indexOffset + numVisibleItems();
}

bool RenderListBox::scroll(ScrollDirection direction, ScrollGranularity granularity, float multiplier, Node**)
{
    return ScrollableArea::scroll(direction, granularity, multiplier);
}

bool RenderListBox::logicalScroll(ScrollLogicalDirection direction, ScrollGranularity granularity, float multiplier, Node**)
{
    return ScrollableArea::scroll(logicalToPhysical(direction, style()->isHorizontalWritingMode(), style()->isFlippedBlocksWritingMode()), granularity, multiplier);
}

void RenderListBox::valueChanged(unsigned listIndex)
{
    HTMLSelectElement* element = selectElement();
    element->setSelectedIndex(element->listToOptionIndex(listIndex));
    element->dispatchFormControlChangeEvent();
}

int RenderListBox::scrollSize(ScrollbarOrientation orientation) const
{
    return ((orientation == VerticalScrollbar) && m_vBar) ? (m_vBar->totalSize() - m_vBar->visibleSize()) : 0;
}

int RenderListBox::scrollPosition(Scrollbar*) const
{
    return m_indexOffset;
}

void RenderListBox::setScrollOffset(const IntPoint& offset)
{
    scrollTo(offset.y());
}

void RenderListBox::scrollTo(int newOffset)
{
    if (newOffset == m_indexOffset)
        return;

    m_indexOffset = newOffset;
    repaint();
    node()->document()->eventQueue()->enqueueOrDispatchScrollEvent(node(), DocumentEventQueue::ScrollEventElementTarget);
}

LayoutUnit RenderListBox::itemHeight() const
{
    return style()->fontMetrics().height() + rowSpacing;
}

int RenderListBox::verticalScrollbarWidth() const
{
    return m_vBar && !m_vBar->isOverlayScrollbar() ? m_vBar->width() : 0;
}

// FIXME: We ignore padding in the vertical direction as far as these values are concerned, since that's
// how the control currently paints.
int RenderListBox::scrollWidth() const
{
    // There is no horizontal scrolling allowed.
    return pixelSnappedClientWidth();
}

int RenderListBox::scrollHeight() const
{
    return max(pixelSnappedClientHeight(), roundToInt(listHeight()));
}

int RenderListBox::scrollLeft() const
{
    return 0;
}

void RenderListBox::setScrollLeft(int)
{
}

int RenderListBox::scrollTop() const
{
    return m_indexOffset * itemHeight();
}

void RenderListBox::setScrollTop(int newTop)
{
    // Determine an index and scroll to it.    
    int index = newTop / itemHeight();
    if (index < 0 || index >= numItems() || index == m_indexOffset)
        return;
    
    scrollToOffsetWithoutAnimation(VerticalScrollbar, index);
}

bool RenderListBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    if (!RenderBlock::nodeAtPoint(request, result, locationInContainer, accumulatedOffset, hitTestAction))
        return false;
    const Vector<HTMLElement*>& listItems = selectElement()->listItems();
    int size = numItems();
    LayoutPoint adjustedLocation = accumulatedOffset + location();

    for (int i = 0; i < size; ++i) {
        if (itemBoundingBoxRect(adjustedLocation, i).contains(locationInContainer.point())) {
            if (Element* node = listItems[i]) {
                result.setInnerNode(node);
                if (!result.innerNonSharedNode())
                    result.setInnerNonSharedNode(node);
                result.setLocalPoint(locationInContainer.point() - toLayoutSize(adjustedLocation));
                break;
            }
        }
    }

    return true;
}

LayoutRect RenderListBox::controlClipRect(const LayoutPoint& additionalOffset) const
{
    LayoutRect clipRect = contentBoxRect();
    clipRect.moveBy(additionalOffset);
    return clipRect;
}

bool RenderListBox::isActive() const
{
    Page* page = frame()->page();
    return page && page->focusController()->isActive();
}

void RenderListBox::invalidateScrollbarRect(Scrollbar* scrollbar, const IntRect& rect)
{
    IntRect scrollRect = rect;
    scrollRect.move(width() - borderRight() - scrollbar->width(), borderTop());
    repaintRectangle(scrollRect);
}

IntRect RenderListBox::convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntRect& scrollbarRect) const
{
    RenderView* view = this->view();
    if (!view)
        return scrollbarRect;

    IntRect rect = scrollbarRect;

    int scrollbarLeft = width() - borderRight() - scrollbar->width();
    int scrollbarTop = borderTop();
    rect.move(scrollbarLeft, scrollbarTop);

    return view->frameView()->convertFromRenderer(this, rect);
}

IntRect RenderListBox::convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntRect& parentRect) const
{
    RenderView* view = this->view();
    if (!view)
        return parentRect;

    IntRect rect = view->frameView()->convertToRenderer(this, parentRect);

    int scrollbarLeft = width() - borderRight() - scrollbar->width();
    int scrollbarTop = borderTop();
    rect.move(-scrollbarLeft, -scrollbarTop);
    return rect;
}

IntPoint RenderListBox::convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntPoint& scrollbarPoint) const
{
    RenderView* view = this->view();
    if (!view)
        return scrollbarPoint;

    IntPoint point = scrollbarPoint;

    int scrollbarLeft = width() - borderRight() - scrollbar->width();
    int scrollbarTop = borderTop();
    point.move(scrollbarLeft, scrollbarTop);

    return view->frameView()->convertFromRenderer(this, point);
}

IntPoint RenderListBox::convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntPoint& parentPoint) const
{
    RenderView* view = this->view();
    if (!view)
        return parentPoint;

    IntPoint point = view->frameView()->convertToRenderer(this, parentPoint);

    int scrollbarLeft = width() - borderRight() - scrollbar->width();
    int scrollbarTop = borderTop();
    point.move(-scrollbarLeft, -scrollbarTop);
    return point;
}

IntSize RenderListBox::contentsSize() const
{
    return IntSize(scrollWidth(), scrollHeight());
}

int RenderListBox::visibleHeight() const
{
    return height();
}

int RenderListBox::visibleWidth() const
{
    return width();
}

IntPoint RenderListBox::lastKnownMousePosition() const
{
    RenderView* view = this->view();
    if (!view)
        return IntPoint();
    return view->frameView()->lastKnownMousePosition();
}

bool RenderListBox::isHandlingWheelEvent() const
{
    RenderView* view = this->view();
    if (!view)
        return false;
    return view->frameView()->isHandlingWheelEvent();
}

bool RenderListBox::shouldSuspendScrollAnimations() const
{
    RenderView* view = this->view();
    if (!view)
        return true;
    return view->frameView()->shouldSuspendScrollAnimations();
}

bool RenderListBox::scrollbarsCanBeActive() const
{
    RenderView* view = this->view();
    if (!view)
        return false;
    return view->frameView()->scrollbarsCanBeActive();
}

bool RenderListBox::scrollbarAnimationsAreSuppressed() const
{
    RenderView* view = this->view();
    if (!view)
        return false;
    return view->frameView()->scrollbarAnimationsAreSuppressed();
}

ScrollableArea* RenderListBox::enclosingScrollableArea() const
{
    // FIXME: Return a RenderLayer that's scrollable.
    return 0;
}

IntRect RenderListBox::scrollableAreaBoundingBox() const
{
    return absoluteBoundingBoxRect();
}

PassRefPtr<Scrollbar> RenderListBox::createScrollbar()
{
    RefPtr<Scrollbar> widget;
    bool hasCustomScrollbarStyle = style()->hasPseudoStyle(SCROLLBAR);
    if (hasCustomScrollbarStyle)
        widget = RenderScrollbar::createCustomScrollbar(this, VerticalScrollbar, this->node());
    else {
        widget = Scrollbar::createNativeScrollbar(this, VerticalScrollbar, theme()->scrollbarControlSizeForPart(ListboxPart));
        didAddScrollbar(widget.get(), VerticalScrollbar);
    }
    document()->view()->addChild(widget.get());        
    return widget.release();
}

void RenderListBox::destroyScrollbar()
{
    if (!m_vBar)
        return;

    if (!m_vBar->isCustomScrollbar())
        ScrollableArea::willRemoveScrollbar(m_vBar.get(), VerticalScrollbar);
    m_vBar->removeFromParent();
    m_vBar->disconnectFromScrollableArea();
    m_vBar = 0;
}

void RenderListBox::setHasVerticalScrollbar(bool hasScrollbar)
{
    if (hasScrollbar == (m_vBar != 0))
        return;

    if (hasScrollbar)
        m_vBar = createScrollbar();
    else
        destroyScrollbar();

    if (m_vBar)
        m_vBar->styleChanged();

    // Force an update since we know the scrollbars have changed things.
#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
    if (document()->hasAnnotatedRegions())
        document()->setAnnotatedRegionsDirty(true);
#endif
}

} // namespace WebCore
