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
#include "CSSStyleSelector.h"
#include "Document.h"
#include "EventHandler.h"
#include "EventQueue.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "NodeRenderStyle.h"
#include "OptionGroupElement.h"
#include "OptionElement.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderLayer.h"
#include "RenderScrollbar.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "Scrollbar.h"
#include "ScrollbarTheme.h"
#include "SelectElement.h"
#include "SelectionController.h"
#include "TextRun.h"
#include <math.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;
 
const int rowSpacing = 1;

const int optionsSpacingHorizontal = 2;

const int minSize = 4;
const int maxDefaultSize = 10;

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
    if (Page* page = frame()->page()) {
        m_page = page;
        m_page->addScrollableArea(this);
    }
}

RenderListBox::~RenderListBox()
{
    setHasVerticalScrollbar(false);
    if (m_page)
        m_page->removeScrollableArea(this);
}

void RenderListBox::updateFromElement()
{
    if (m_optionsChanged) {
        const Vector<Element*>& listItems = toSelectElement(static_cast<Element*>(node()))->listItems();
        int size = numItems();
        
        float width = 0;
        for (int i = 0; i < size; ++i) {
            Element* element = listItems[i];
            String text;
            Font itemFont = style()->font();
            if (OptionElement* optionElement = toOptionElement(element))
                text = optionElement->textIndentedToRespectGroupLabel();
            else if (OptionGroupElement* optionGroupElement = toOptionGroupElement(element)) {
                text = optionGroupElement->groupLabelText();
                FontDescription d = itemFont.fontDescription();
                d.setWeight(d.bolderWeight());
                itemFont = Font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
                itemFont.update(document()->styleSelector()->fontSelector());
            }

            if (!text.isEmpty()) {
                // FIXME: Why is this always LTR? Can't text direction affect the width?
                float textWidth = itemFont.width(TextRun(text.impl(), false, 0, 0, TextRun::AllowTrailingExpansion, LTR));
                width = max(width, textWidth);
            }
        }
        m_optionsWidth = static_cast<int>(ceilf(width));
        m_optionsChanged = false;
        
        setHasVerticalScrollbar(true);

        setNeedsLayoutAndPrefWidthsRecalc();
    }
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
    
    if (AXObjectCache::accessibilityEnabled())
        document()->axObjectCache()->selectedChildrenChanged(this);
}

void RenderListBox::layout()
{
    RenderBlock::layout();
    if (m_scrollToRevealSelectionAfterLayout) {
        view()->disableLayoutState();
        scrollToRevealSelection();
        view()->enableLayoutState();
    }
}

void RenderListBox::scrollToRevealSelection()
{    
    SelectElement* select = toSelectElement(static_cast<Element*>(node()));

    m_scrollToRevealSelectionAfterLayout = false;

    int firstIndex = select->activeSelectionStartListIndex();
    if (firstIndex >= 0 && !listIndexIsVisible(select->activeSelectionEndListIndex()))
        scrollToRevealElementAtListIndex(firstIndex);
}

void RenderListBox::computePreferredLogicalWidths()
{
    ASSERT(!m_optionsChanged);

    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    if (style()->width().isFixed() && style()->width().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = computeContentBoxLogicalWidth(style()->width().value());
    else {
        m_maxPreferredLogicalWidth = m_optionsWidth + 2 * optionsSpacingHorizontal;
        if (m_vBar)
            m_maxPreferredLogicalWidth += m_vBar->width();
    }

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

int RenderListBox::size() const
{
    int specifiedSize = toSelectElement(static_cast<Element*>(node()))->size();
    if (specifiedSize > 1)
        return max(minSize, specifiedSize);
    return min(max(minSize, numItems()), maxDefaultSize);
}

int RenderListBox::numVisibleItems() const
{
    // Only count fully visible rows. But don't return 0 even if only part of a row shows.
    return max(1, (contentHeight() + rowSpacing) / itemHeight());
}

int RenderListBox::numItems() const
{
    return toSelectElement(static_cast<Element*>(node()))->listItems().size();
}

int RenderListBox::listHeight() const
{
    return itemHeight() * numItems() - rowSpacing;
}

void RenderListBox::computeLogicalHeight()
{
    int toAdd = borderAndPaddingHeight();
 
    int itemHeight = RenderListBox::itemHeight();
    setHeight(itemHeight * size() - rowSpacing + toAdd);
    
    RenderBlock::computeLogicalHeight();
    
    if (m_vBar) {
        bool enabled = numVisibleItems() < numItems();
        m_vBar->setEnabled(enabled);
        m_vBar->setSteps(1, min(1, numVisibleItems() - 1), itemHeight);
        m_vBar->setProportion(numVisibleItems(), numItems());
        if (!enabled)
            m_indexOffset = 0;
    }
}

int RenderListBox::baselinePosition(FontBaseline baselineType, bool firstLine, LineDirectionMode lineDirection, LinePositionMode linePositionMode) const
{
    return RenderBox::baselinePosition(baselineType, firstLine, lineDirection, linePositionMode) - baselineAdjustment;
}

IntRect RenderListBox::itemBoundingBoxRect(int tx, int ty, int index)
{
    return IntRect(tx + borderLeft() + paddingLeft(),
                   ty + borderTop() + paddingTop() + itemHeight() * (index - m_indexOffset),
                   contentWidth(), itemHeight());
}
    
void RenderListBox::paintObject(PaintInfo& paintInfo, int tx, int ty)
{
    if (style()->visibility() != VISIBLE)
        return;
    
    int listItemsSize = numItems();

    if (paintInfo.phase == PaintPhaseForeground) {
        int index = m_indexOffset;
        while (index < listItemsSize && index <= m_indexOffset + numVisibleItems()) {
            paintItemForeground(paintInfo, tx, ty, index);
            index++;
        }
    }

    // Paint the children.
    RenderBlock::paintObject(paintInfo, tx, ty);

    switch (paintInfo.phase) {
    // Depending on whether we have overlay scrollbars they
    // get rendered in the foreground or background phases
    case PaintPhaseForeground:
        if (m_vBar->isOverlayScrollbar())
            paintScrollbar(paintInfo, tx, ty);
        break;
    case PaintPhaseBlockBackground:
        if (!m_vBar->isOverlayScrollbar())
            paintScrollbar(paintInfo, tx, ty);
        break;
    case PaintPhaseChildBlockBackground:
    case PaintPhaseChildBlockBackgrounds: {
        int index = m_indexOffset;
        while (index < listItemsSize && index <= m_indexOffset + numVisibleItems()) {
            paintItemBackground(paintInfo, tx, ty, index);
            index++;
        }
        break;
    }
    default:
        break;
    }
}

void RenderListBox::addFocusRingRects(Vector<IntRect>& rects, int tx, int ty)
{
    if (!isSpatialNavigationEnabled(frame()))
        return RenderBlock::addFocusRingRects(rects, tx, ty);

    SelectElement* select = toSelectElement(static_cast<Element*>(node()));

    // Focus the last selected item.
    int selectedItem = select->activeSelectionEndListIndex();
    if (selectedItem >= 0) {
        rects.append(itemBoundingBoxRect(tx, ty, selectedItem));
        return;
    }

    // No selected items, find the first non-disabled item.
    int size = numItems();
    const Vector<Element*>& listItems = select->listItems();
    for (int i = 0; i < size; ++i) {
        OptionElement* optionElement = toOptionElement(listItems[i]);
        if (optionElement && !optionElement->disabled()) {
            rects.append(itemBoundingBoxRect(tx, ty, i));
            return;
        }
    }
}

void RenderListBox::paintScrollbar(PaintInfo& paintInfo, int tx, int ty)
{
    if (m_vBar) {
        IntRect scrollRect(tx + width() - borderRight() - m_vBar->width(),
                           ty + borderTop(),
                           m_vBar->width(),
                           height() - (borderTop() + borderBottom()));
        m_vBar->setFrameRect(scrollRect);
        m_vBar->paint(paintInfo.context, paintInfo.rect);
    }
}

static IntSize itemOffsetForAlignment(TextRun textRun, RenderStyle* itemStyle, Font itemFont, IntRect itemBoudingBox)
{
    ETextAlign actualAlignment = itemStyle->textAlign();
    // FIXME: Firefox doesn't respect JUSTIFY. Should we?
    if (actualAlignment == TAAUTO || actualAlignment == JUSTIFY)
      actualAlignment = itemStyle->isLeftToRightDirection() ? LEFT : RIGHT;

    IntSize offset = IntSize(0, itemFont.fontMetrics().ascent());
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

void RenderListBox::paintItemForeground(PaintInfo& paintInfo, int tx, int ty, int listIndex)
{
    SelectElement* select = toSelectElement(static_cast<Element*>(node()));
    const Vector<Element*>& listItems = select->listItems();
    Element* element = listItems[listIndex];
    OptionElement* optionElement = toOptionElement(element);

    RenderStyle* itemStyle = element->renderStyle();
    if (!itemStyle)
        itemStyle = style();

    if (itemStyle->visibility() == HIDDEN)
        return;

    String itemText;
    if (optionElement)
        itemText = optionElement->textIndentedToRespectGroupLabel();
    else if (OptionGroupElement* optionGroupElement = toOptionGroupElement(element))
        itemText = optionGroupElement->groupLabelText();
    
    Color textColor = element->renderStyle() ? element->renderStyle()->visitedDependentColor(CSSPropertyColor) : style()->visitedDependentColor(CSSPropertyColor);
    if (optionElement && optionElement->selected()) {
        if (frame()->selection()->isFocusedAndActive() && document()->focusedNode() == node())
            textColor = theme()->activeListBoxSelectionForegroundColor();
        // Honor the foreground color for disabled items
        else if (!element->disabled())
            textColor = theme()->inactiveListBoxSelectionForegroundColor();
    }

    ColorSpace colorSpace = itemStyle->colorSpace();
    paintInfo.context->setFillColor(textColor, colorSpace);

    unsigned length = itemText.length();
    const UChar* string = itemText.characters();
    TextRun textRun(string, length, false, 0, 0, TextRun::AllowTrailingExpansion, itemStyle->direction(), itemStyle->unicodeBidi() == Override);
    Font itemFont = style()->font();
    IntRect r = itemBoundingBoxRect(tx, ty, listIndex);
    r.move(itemOffsetForAlignment(textRun, itemStyle, itemFont, r));

    if (isOptionGroupElement(element)) {
        FontDescription d = itemFont.fontDescription();
        d.setWeight(d.bolderWeight());
        itemFont = Font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
        itemFont.update(document()->styleSelector()->fontSelector());
    }

    // Draw the item text
    if (itemStyle->visibility() != HIDDEN)
        paintInfo.context->drawBidiText(itemFont, textRun, r.location());
}

void RenderListBox::paintItemBackground(PaintInfo& paintInfo, int tx, int ty, int listIndex)
{
    SelectElement* select = toSelectElement(static_cast<Element*>(node()));
    const Vector<Element*>& listItems = select->listItems();
    Element* element = listItems[listIndex];
    OptionElement* optionElement = toOptionElement(element);

    Color backColor;
    if (optionElement && optionElement->selected()) {
        if (frame()->selection()->isFocusedAndActive() && document()->focusedNode() == node())
            backColor = theme()->activeListBoxSelectionBackgroundColor();
        else
            backColor = theme()->inactiveListBoxSelectionBackgroundColor();
    } else
        backColor = element->renderStyle() ? element->renderStyle()->visitedDependentColor(CSSPropertyBackgroundColor) : style()->visitedDependentColor(CSSPropertyBackgroundColor);

    // Draw the background for this list box item
    if (!element->renderStyle() || element->renderStyle()->visibility() != HIDDEN) {
        ColorSpace colorSpace = element->renderStyle() ? element->renderStyle()->colorSpace() : style()->colorSpace();
        IntRect itemRect = itemBoundingBoxRect(tx, ty, listIndex);
        itemRect.intersect(controlClipRect(tx, ty));
        paintInfo.context->fillRect(itemRect, backColor, colorSpace);
    }
}

bool RenderListBox::isPointInOverflowControl(HitTestResult& result, int _x, int _y, int _tx, int _ty)
{
    if (!m_vBar)
        return false;

    IntRect vertRect(_tx + width() - borderRight() - m_vBar->width(),
                     _ty + borderTop(),
                     m_vBar->width(),
                     height() - borderTop() - borderBottom());

    if (vertRect.contains(_x, _y)) {
        result.setScrollbar(m_vBar.get());
        return true;
    }
    return false;
}

int RenderListBox::listIndexAtOffset(int offsetX, int offsetY)
{
    if (!numItems())
        return -1;

    if (offsetY < borderTop() + paddingTop() || offsetY > height() - paddingBottom() - borderBottom())
        return -1;

    int scrollbarWidth = m_vBar ? m_vBar->width() : 0;
    if (offsetX < borderLeft() + paddingLeft() || offsetX > width() - borderRight() - paddingRight() - scrollbarWidth)
        return -1;

    int newOffset = (offsetY - borderTop() - paddingTop()) / itemHeight() + m_indexOffset;
    return newOffset < numItems() ? newOffset : -1;
}

void RenderListBox::panScroll(const IntPoint& panStartMousePosition)
{
    const int maxSpeed = 20;
    const int iconRadius = 7;
    const int speedReducer = 4;

    // FIXME: This doesn't work correctly with transforms.
    FloatPoint absOffset = localToAbsolute();

    IntPoint currentMousePosition = frame()->eventHandler()->currentMousePosition();
    // We need to check if the current mouse position is out of the window. When the mouse is out of the window, the position is incoherent
    static IntPoint previousMousePosition;
    if (currentMousePosition.y() < 0)
        currentMousePosition = previousMousePosition;
    else
        previousMousePosition = currentMousePosition;

    int yDelta = currentMousePosition.y() - panStartMousePosition.y();

    // If the point is too far from the center we limit the speed
    yDelta = max(min(yDelta, maxSpeed), -maxSpeed);
    
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
    SelectElement* select = toSelectElement(static_cast<Element*>(node()));
    select->updateListBoxSelection(!select->multiple());
    m_inAutoscroll = false;
}

int RenderListBox::scrollToward(const IntPoint& destination)
{
    // FIXME: This doesn't work correctly with transforms.
    FloatPoint absPos = localToAbsolute();
    int offsetX = destination.x() - absPos.x();
    int offsetY = destination.y() - absPos.y();

    int rows = numVisibleItems();
    int offset = m_indexOffset;
    
    if (offsetY < borderTop() + paddingTop() && scrollToRevealElementAtListIndex(offset - 1))
        return offset - 1;
    
    if (offsetY > height() - paddingBottom() - borderBottom() && scrollToRevealElementAtListIndex(offset + rows))
        return offset + rows - 1;
    
    return listIndexAtOffset(offsetX, offsetY);
}

void RenderListBox::autoscroll()
{
    IntPoint pos = frame()->view()->windowToContents(frame()->eventHandler()->currentMousePosition());

    int endIndex = scrollToward(pos);
    if (endIndex >= 0) {
        SelectElement* select = toSelectElement(static_cast<Element*>(node()));
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
    toSelectElement(static_cast<Element*>(node()))->listBoxOnChange();
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

    ScrollableArea::scrollToYOffsetWithoutAnimation(newOffset);

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
    Element* element = static_cast<Element*>(node());
    SelectElement* select = toSelectElement(element);
    select->setSelectedIndex(select->listToOptionIndex(listIndex));
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
    node()->document()->eventQueue()->enqueueOrDispatchScrollEvent(node(), EventQueue::ScrollEventElementTarget);
}

int RenderListBox::itemHeight() const
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
    return clientWidth();
}

int RenderListBox::scrollHeight() const
{
    return max(clientHeight(), listHeight());
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
    
    ScrollableArea::scrollToYOffsetWithoutAnimation(index);
}

bool RenderListBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, HitTestAction hitTestAction)
{
    if (!RenderBlock::nodeAtPoint(request, result, x, y, tx, ty, hitTestAction))
        return false;
    const Vector<Element*>& listItems = toSelectElement(static_cast<Element*>(node()))->listItems();
    int size = numItems();
    tx += this->x();
    ty += this->y();
    for (int i = 0; i < size; ++i) {
        if (itemBoundingBoxRect(tx, ty, i).contains(x, y)) {
            if (Element* node = listItems[i]) {
                result.setInnerNode(node);
                if (!result.innerNonSharedNode())
                    result.setInnerNonSharedNode(node);
                result.setLocalPoint(IntPoint(x - tx, y - ty));
                break;
            }
        }
    }

    return true;
}

IntRect RenderListBox::controlClipRect(int tx, int ty) const
{
    IntRect clipRect = contentBoxRect();
    clipRect.move(tx, ty);
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

IntPoint RenderListBox::currentMousePosition() const
{
    RenderView* view = this->view();
    if (!view)
        return IntPoint();
    return view->frameView()->currentMousePosition();
}

bool RenderListBox::shouldSuspendScrollAnimations() const
{
    RenderView* view = this->view();
    if (!view)
        return true;
    return view->frameView()->shouldSuspendScrollAnimations();
}

PassRefPtr<Scrollbar> RenderListBox::createScrollbar()
{
    RefPtr<Scrollbar> widget;
    bool hasCustomScrollbarStyle = style()->hasPseudoStyle(SCROLLBAR);
    if (hasCustomScrollbarStyle)
        widget = RenderScrollbar::createCustomScrollbar(this, VerticalScrollbar, this);
    else {
        widget = Scrollbar::createNativeScrollbar(this, VerticalScrollbar, theme()->scrollbarControlSizeForPart(ListboxPart));
        didAddVerticalScrollbar(widget.get());
    }
    document()->view()->addChild(widget.get());        
    return widget.release();
}

void RenderListBox::destroyScrollbar()
{
    if (!m_vBar)
        return;

    if (!m_vBar->isCustomScrollbar())
        ScrollableArea::willRemoveVerticalScrollbar(m_vBar.get());
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

#if ENABLE(DASHBOARD_SUPPORT)
    // Force an update since we know the scrollbars have changed things.
    if (document()->hasDashboardRegions())
        document()->setDashboardRegionsDirty(true);
#endif
}

} // namespace WebCore
