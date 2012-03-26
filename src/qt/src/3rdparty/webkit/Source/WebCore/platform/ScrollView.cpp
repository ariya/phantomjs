/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "ScrollView.h"

#include "AXObjectCache.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "HostWindow.h"
#include "PlatformMouseEvent.h"
#include "PlatformWheelEvent.h"
#include "ScrollAnimator.h"
#include "Scrollbar.h"
#include "ScrollbarTheme.h"
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

ScrollView::ScrollView()
    : m_horizontalScrollbarMode(ScrollbarAuto)
    , m_verticalScrollbarMode(ScrollbarAuto)
    , m_horizontalScrollbarLock(false)
    , m_verticalScrollbarLock(false)
    , m_prohibitsScrolling(false)
    , m_canBlitOnScroll(true)
    , m_scrollbarsAvoidingResizer(0)
    , m_scrollbarsSuppressed(false)
    , m_inUpdateScrollbars(false)
    , m_updateScrollbarsPass(0)
    , m_drawPanScrollIcon(false)
    , m_useFixedLayout(false)
    , m_paintsEntireContents(false)
    , m_clipsRepaints(true)
    , m_delegatesScrolling(false)
    , m_containsScrollableAreaWithOverlayScrollbars(false)
{
    platformInit();
}

ScrollView::~ScrollView()
{
    platformDestroy();
}

void ScrollView::addChild(PassRefPtr<Widget> prpChild) 
{
    Widget* child = prpChild.get();
    ASSERT(child != this && !child->parent());
    child->setParent(this);
    m_children.add(prpChild);
    if (child->platformWidget())
        platformAddChild(child);
}

void ScrollView::removeChild(Widget* child)
{
    ASSERT(child->parent() == this);
    child->setParent(0);
    m_children.remove(child);
    if (child->platformWidget())
        platformRemoveChild(child);
}

void ScrollView::setHasHorizontalScrollbar(bool hasBar)
{
    if (hasBar && avoidScrollbarCreation())
        return;

    if (hasBar && !m_horizontalScrollbar) {
        m_horizontalScrollbar = createScrollbar(HorizontalScrollbar);
        addChild(m_horizontalScrollbar.get());
        didAddHorizontalScrollbar(m_horizontalScrollbar.get());
        m_horizontalScrollbar->styleChanged();
    } else if (!hasBar && m_horizontalScrollbar) {
        willRemoveHorizontalScrollbar(m_horizontalScrollbar.get());
        removeChild(m_horizontalScrollbar.get());
        m_horizontalScrollbar = 0;
    }
    
    if (AXObjectCache::accessibilityEnabled() && axObjectCache())
        axObjectCache()->handleScrollbarUpdate(this);
}

void ScrollView::setHasVerticalScrollbar(bool hasBar)
{
    if (hasBar && avoidScrollbarCreation())
        return;

    if (hasBar && !m_verticalScrollbar) {
        m_verticalScrollbar = createScrollbar(VerticalScrollbar);
        addChild(m_verticalScrollbar.get());
        didAddVerticalScrollbar(m_verticalScrollbar.get());
        m_verticalScrollbar->styleChanged();
    } else if (!hasBar && m_verticalScrollbar) {
        willRemoveVerticalScrollbar(m_verticalScrollbar.get());
        removeChild(m_verticalScrollbar.get());
        m_verticalScrollbar = 0;
    }
    
    if (AXObjectCache::accessibilityEnabled() && axObjectCache())
        axObjectCache()->handleScrollbarUpdate(this);
}

#if !PLATFORM(GTK)
PassRefPtr<Scrollbar> ScrollView::createScrollbar(ScrollbarOrientation orientation)
{
    return Scrollbar::createNativeScrollbar(this, orientation, RegularScrollbar);
}

void ScrollView::setScrollbarModes(ScrollbarMode horizontalMode, ScrollbarMode verticalMode,
                                   bool horizontalLock, bool verticalLock)
{
    bool needsUpdate = false;

    if (horizontalMode != horizontalScrollbarMode() && !m_horizontalScrollbarLock) {
        m_horizontalScrollbarMode = horizontalMode;
        needsUpdate = true;
    }

    if (verticalMode != verticalScrollbarMode() && !m_verticalScrollbarLock) {
        m_verticalScrollbarMode = verticalMode;
        needsUpdate = true;
    }

    if (horizontalLock)
        setHorizontalScrollbarLock();

    if (verticalLock)
        setVerticalScrollbarLock();

    if (!needsUpdate)
        return;

    if (platformWidget())
        platformSetScrollbarModes();
    else
        updateScrollbars(scrollOffset());
}
#endif

void ScrollView::scrollbarModes(ScrollbarMode& horizontalMode, ScrollbarMode& verticalMode) const
{
    if (platformWidget()) {
        platformScrollbarModes(horizontalMode, verticalMode);
        return;
    }
    horizontalMode = m_horizontalScrollbarMode;
    verticalMode = m_verticalScrollbarMode;
}

void ScrollView::setCanHaveScrollbars(bool canScroll)
{
    ScrollbarMode newHorizontalMode;
    ScrollbarMode newVerticalMode;
    
    scrollbarModes(newHorizontalMode, newVerticalMode);
    
    if (canScroll && newVerticalMode == ScrollbarAlwaysOff)
        newVerticalMode = ScrollbarAuto;
    else if (!canScroll)
        newVerticalMode = ScrollbarAlwaysOff;
    
    if (canScroll && newHorizontalMode == ScrollbarAlwaysOff)
        newHorizontalMode = ScrollbarAuto;
    else if (!canScroll)
        newHorizontalMode = ScrollbarAlwaysOff;
    
    setScrollbarModes(newHorizontalMode, newVerticalMode);
}

void ScrollView::setCanBlitOnScroll(bool b)
{
    if (platformWidget()) {
        platformSetCanBlitOnScroll(b);
        return;
    }

    m_canBlitOnScroll = b;
}

bool ScrollView::canBlitOnScroll() const
{
    if (platformWidget())
        return platformCanBlitOnScroll();

    return m_canBlitOnScroll;
}

void ScrollView::setPaintsEntireContents(bool paintsEntireContents)
{
    m_paintsEntireContents = paintsEntireContents;
}

void ScrollView::setClipsRepaints(bool clipsRepaints)
{
    m_clipsRepaints = clipsRepaints;
}

void ScrollView::setDelegatesScrolling(bool delegatesScrolling)
{
    m_delegatesScrolling = delegatesScrolling;
}

#if !PLATFORM(GTK)
IntRect ScrollView::visibleContentRect(bool includeScrollbars) const
{
    if (platformWidget())
        return platformVisibleContentRect(includeScrollbars);

#if !PLATFORM(EFL)
    if (paintsEntireContents())
        return IntRect(IntPoint(0, 0), contentsSize());
#endif

    int verticalScrollbarWidth = verticalScrollbar() && !verticalScrollbar()->isOverlayScrollbar()
        && !includeScrollbars ? verticalScrollbar()->width() : 0;
    int horizontalScrollbarHeight = horizontalScrollbar() && !horizontalScrollbar()->isOverlayScrollbar()
        && !includeScrollbars ? horizontalScrollbar()->height() : 0;

    return IntRect(IntPoint(m_scrollOffset.width(), m_scrollOffset.height()),
                   IntSize(max(0, m_boundsSize.width() - verticalScrollbarWidth), 
                           max(0, m_boundsSize.height() - horizontalScrollbarHeight)));
}
#endif

int ScrollView::layoutWidth() const
{
    return m_fixedLayoutSize.isEmpty() || !m_useFixedLayout ? visibleWidth() : m_fixedLayoutSize.width();
}

int ScrollView::layoutHeight() const
{
    return m_fixedLayoutSize.isEmpty() || !m_useFixedLayout ? visibleHeight() : m_fixedLayoutSize.height();
}

IntSize ScrollView::fixedLayoutSize() const
{
    return m_fixedLayoutSize;
}

void ScrollView::setFixedLayoutSize(const IntSize& newSize)
{
    if (fixedLayoutSize() == newSize)
        return;
    m_fixedLayoutSize = newSize;
    updateScrollbars(scrollOffset());
}

bool ScrollView::useFixedLayout() const
{
    return m_useFixedLayout;
}

void ScrollView::setUseFixedLayout(bool enable)
{
    if (useFixedLayout() == enable)
        return;
    m_useFixedLayout = enable;
    updateScrollbars(scrollOffset());
}

IntSize ScrollView::contentsSize() const
{
    if (platformWidget())
        return platformContentsSize();
    return m_contentsSize;
}

void ScrollView::setContentsSize(const IntSize& newSize)
{
    if (contentsSize() == newSize)
        return;
    m_contentsSize = newSize;
    if (platformWidget())
        platformSetContentsSize();
    else
        updateScrollbars(scrollOffset());
}

IntPoint ScrollView::maximumScrollPosition() const
{
    IntPoint maximumOffset(contentsWidth() - visibleWidth() - m_scrollOrigin.x(), contentsHeight() - visibleHeight() - m_scrollOrigin.y());
    maximumOffset.clampNegativeToZero();
    return maximumOffset;
}

IntPoint ScrollView::minimumScrollPosition() const
{
    return IntPoint(-m_scrollOrigin.x(), -m_scrollOrigin.y());
}

IntPoint ScrollView::adjustScrollPositionWithinRange(const IntPoint& scrollPoint) const
{
    IntPoint newScrollPosition = scrollPoint.shrunkTo(maximumScrollPosition());
    newScrollPosition = newScrollPosition.expandedTo(minimumScrollPosition());
    return newScrollPosition;
}

int ScrollView::scrollSize(ScrollbarOrientation orientation) const
{
    Scrollbar* scrollbar = ((orientation == HorizontalScrollbar) ? m_horizontalScrollbar : m_verticalScrollbar).get();
    return scrollbar ? (scrollbar->totalSize() - scrollbar->visibleSize()) : 0;
}

void ScrollView::didCompleteRubberBand(const IntSize&) const
{
}

void ScrollView::notifyPageThatContentAreaWillPaint() const
{
}

void ScrollView::setScrollOffset(const IntPoint& offset)
{
    int horizontalOffset = offset.x();
    int verticalOffset = offset.y();
    if (constrainsScrollingToContentEdge()) {
        horizontalOffset = max(min(horizontalOffset, contentsWidth() - visibleWidth()), 0);
        verticalOffset = max(min(verticalOffset, contentsHeight() - visibleHeight()), 0);
    }

    IntSize newOffset = m_scrollOffset;
    newOffset.setWidth(horizontalOffset - m_scrollOrigin.x());
    newOffset.setHeight(verticalOffset - m_scrollOrigin.y());

    scrollTo(newOffset);
}

void ScrollView::scrollTo(const IntSize& newOffset)
{
    IntSize scrollDelta = newOffset - m_scrollOffset;
    if (scrollDelta == IntSize())
        return;
    m_scrollOffset = newOffset;

    if (scrollbarsSuppressed())
        return;

    repaintFixedElementsAfterScrolling();
    scrollContents(scrollDelta);
}

int ScrollView::scrollPosition(Scrollbar* scrollbar) const
{
    if (scrollbar->orientation() == HorizontalScrollbar)
        return scrollPosition().x() + m_scrollOrigin.x();
    if (scrollbar->orientation() == VerticalScrollbar)
        return scrollPosition().y() + m_scrollOrigin.y();
    return 0;
}

void ScrollView::setScrollPosition(const IntPoint& scrollPoint)
{
    if (prohibitsScrolling())
        return;

    if (platformWidget()) {
        platformSetScrollPosition(scrollPoint);
        return;
    }

#if ENABLE(TILED_BACKING_STORE)
    if (delegatesScrolling()) {
        hostWindow()->delegatedScrollRequested(scrollPoint);
        if (!m_actualVisibleContentRect.isEmpty())
            m_actualVisibleContentRect.setLocation(scrollPoint);
        return;
    }
#endif

    IntPoint newScrollPosition = adjustScrollPositionWithinRange(scrollPoint);

    if (newScrollPosition == scrollPosition())
        return;

    updateScrollbars(IntSize(newScrollPosition.x(), newScrollPosition.y()));
}

bool ScrollView::scroll(ScrollDirection direction, ScrollGranularity granularity)
{
    if (platformWidget())
        return platformScroll(direction, granularity);

    return ScrollableArea::scroll(direction, granularity);
}

bool ScrollView::logicalScroll(ScrollLogicalDirection direction, ScrollGranularity granularity)
{
    return scroll(logicalToPhysical(direction, isVerticalDocument(), isFlippedDocument()), granularity);
}

IntSize ScrollView::overhangAmount() const
{
    IntSize stretch;

    int physicalScrollY = scrollPosition().y() + m_scrollOrigin.y();
    if (physicalScrollY < 0)
        stretch.setHeight(physicalScrollY);
    else if (physicalScrollY > contentsHeight() - visibleContentRect().height())
        stretch.setHeight(physicalScrollY - (contentsHeight() - visibleContentRect().height()));

    int physicalScrollX = scrollPosition().x() + m_scrollOrigin.x();
    if (physicalScrollX < 0)
        stretch.setWidth(physicalScrollX);
    else if (physicalScrollX > contentsWidth() - visibleContentRect().width())
        stretch.setWidth(physicalScrollX - (contentsWidth() - visibleContentRect().width()));

    return stretch;
}

void ScrollView::windowResizerRectChanged()
{
    if (platformWidget())
        return;

    updateScrollbars(scrollOffset());
}

static const unsigned cMaxUpdateScrollbarsPass = 2;

void ScrollView::updateScrollbars(const IntSize& desiredOffset)
{
    if (m_inUpdateScrollbars || prohibitsScrolling() || delegatesScrolling() || platformWidget())
        return;

    // If we came in here with the view already needing a layout, then go ahead and do that
    // first.  (This will be the common case, e.g., when the page changes due to window resizing for example).
    // This layout will not re-enter updateScrollbars and does not count towards our max layout pass total.
    if (!m_scrollbarsSuppressed) {
        m_inUpdateScrollbars = true;
        visibleContentsResized();
        m_inUpdateScrollbars = false;
    }

    bool hasHorizontalScrollbar = m_horizontalScrollbar;
    bool hasVerticalScrollbar = m_verticalScrollbar;
    
    bool newHasHorizontalScrollbar = hasHorizontalScrollbar;
    bool newHasVerticalScrollbar = hasVerticalScrollbar;
   
    ScrollbarMode hScroll = m_horizontalScrollbarMode;
    ScrollbarMode vScroll = m_verticalScrollbarMode;

    if (hScroll != ScrollbarAuto)
        newHasHorizontalScrollbar = (hScroll == ScrollbarAlwaysOn);
    if (vScroll != ScrollbarAuto)
        newHasVerticalScrollbar = (vScroll == ScrollbarAlwaysOn);

    if (m_scrollbarsSuppressed || (hScroll != ScrollbarAuto && vScroll != ScrollbarAuto)) {
        if (hasHorizontalScrollbar != newHasHorizontalScrollbar)
            setHasHorizontalScrollbar(newHasHorizontalScrollbar);
        if (hasVerticalScrollbar != newHasVerticalScrollbar)
            setHasVerticalScrollbar(newHasVerticalScrollbar);
    } else {
        bool sendContentResizedNotification = false;
        
        IntSize docSize = contentsSize();
        IntSize frameSize = m_boundsSize;

        if (hScroll == ScrollbarAuto) {
            newHasHorizontalScrollbar = docSize.width() > visibleWidth();
            if (newHasHorizontalScrollbar && !m_updateScrollbarsPass && docSize.width() <= frameSize.width() && docSize.height() <= frameSize.height())
                newHasHorizontalScrollbar = false;
        }
        if (vScroll == ScrollbarAuto) {
            newHasVerticalScrollbar = docSize.height() > visibleHeight();
            if (newHasVerticalScrollbar && !m_updateScrollbarsPass && docSize.width() <= frameSize.width() && docSize.height() <= frameSize.height())
                newHasVerticalScrollbar = false;
        }

        // If we ever turn one scrollbar off, always turn the other one off too.  Never ever
        // try to both gain/lose a scrollbar in the same pass.
        if (!newHasHorizontalScrollbar && hasHorizontalScrollbar && vScroll != ScrollbarAlwaysOn)
            newHasVerticalScrollbar = false;
        if (!newHasVerticalScrollbar && hasVerticalScrollbar && hScroll != ScrollbarAlwaysOn)
            newHasHorizontalScrollbar = false;

        if (hasHorizontalScrollbar != newHasHorizontalScrollbar) {
            if (m_scrollOrigin.y() && !newHasHorizontalScrollbar)
                m_scrollOrigin.setY(m_scrollOrigin.y() - m_horizontalScrollbar->height());
            setHasHorizontalScrollbar(newHasHorizontalScrollbar);
            sendContentResizedNotification = true;
        }

        if (hasVerticalScrollbar != newHasVerticalScrollbar) {
            if (m_scrollOrigin.x() && !newHasVerticalScrollbar)
                m_scrollOrigin.setX(m_scrollOrigin.x() - m_verticalScrollbar->width());
            setHasVerticalScrollbar(newHasVerticalScrollbar);
            sendContentResizedNotification = true;
        }

        if (sendContentResizedNotification && m_updateScrollbarsPass < cMaxUpdateScrollbarsPass) {
            m_updateScrollbarsPass++;
            contentsResized();
            visibleContentsResized();
            IntSize newDocSize = contentsSize();
            if (newDocSize == docSize) {
                // The layout with the new scroll state had no impact on
                // the document's overall size, so updateScrollbars didn't get called.
                // Recur manually.
                updateScrollbars(desiredOffset);
            }
            m_updateScrollbarsPass--;
        }
    }
    
    // Set up the range (and page step/line step), but only do this if we're not in a nested call (to avoid
    // doing it multiple times).
    if (m_updateScrollbarsPass)
        return;

    m_inUpdateScrollbars = true;

    IntPoint scrollPoint = adjustScrollPositionWithinRange(IntPoint(desiredOffset));
    IntSize scroll(scrollPoint.x(), scrollPoint.y());

    if (m_horizontalScrollbar) {
        int clientWidth = visibleWidth();
        m_horizontalScrollbar->setEnabled(contentsWidth() > clientWidth);
        int pageStep = max(max<int>(clientWidth * Scrollbar::minFractionToStepWhenPaging(), clientWidth - Scrollbar::maxOverlapBetweenPages()), 1);
        IntRect oldRect(m_horizontalScrollbar->frameRect());
        IntRect hBarRect = IntRect(0,
                                   m_boundsSize.height() - m_horizontalScrollbar->height(),
                                   m_boundsSize.width() - (m_verticalScrollbar ? m_verticalScrollbar->width() : 0),
                                   m_horizontalScrollbar->height());
        m_horizontalScrollbar->setFrameRect(hBarRect);
        if (!m_scrollbarsSuppressed && oldRect != m_horizontalScrollbar->frameRect())
            m_horizontalScrollbar->invalidate();

        if (m_scrollbarsSuppressed)
            m_horizontalScrollbar->setSuppressInvalidation(true);
        m_horizontalScrollbar->setSteps(Scrollbar::pixelsPerLineStep(), pageStep);
        m_horizontalScrollbar->setProportion(clientWidth, contentsWidth());
        if (m_scrollbarsSuppressed)
            m_horizontalScrollbar->setSuppressInvalidation(false); 
    } 

    if (m_verticalScrollbar) {
        int clientHeight = visibleHeight();
        m_verticalScrollbar->setEnabled(contentsHeight() > clientHeight);
        int pageStep = max(max<int>(clientHeight * Scrollbar::minFractionToStepWhenPaging(), clientHeight - Scrollbar::maxOverlapBetweenPages()), 1);
        IntRect oldRect(m_verticalScrollbar->frameRect());
        IntRect vBarRect = IntRect(m_boundsSize.width() - m_verticalScrollbar->width(), 
                                   0,
                                   m_verticalScrollbar->width(),
                                   m_boundsSize.height() - (m_horizontalScrollbar ? m_horizontalScrollbar->height() : 0));
        m_verticalScrollbar->setFrameRect(vBarRect);
        if (!m_scrollbarsSuppressed && oldRect != m_verticalScrollbar->frameRect())
            m_verticalScrollbar->invalidate();

        if (m_scrollbarsSuppressed)
            m_verticalScrollbar->setSuppressInvalidation(true);
        m_verticalScrollbar->setSteps(Scrollbar::pixelsPerLineStep(), pageStep);
        m_verticalScrollbar->setProportion(clientHeight, contentsHeight());
        if (m_scrollbarsSuppressed)
            m_verticalScrollbar->setSuppressInvalidation(false);
    }

    if (hasHorizontalScrollbar != (m_horizontalScrollbar != 0) || hasVerticalScrollbar != (m_verticalScrollbar != 0)) {
        // FIXME: Is frameRectsChanged really necessary here? Have any frame rects changed?
        frameRectsChanged();
        positionScrollbarLayers();
        updateScrollCorner();
    }

    ScrollableArea::scrollToOffsetWithoutAnimation(FloatPoint(scroll.width() + m_scrollOrigin.x(), scroll.height() + m_scrollOrigin.y()));

    // Make sure the scrollbar offsets are up to date.
    if (m_horizontalScrollbar)
        m_horizontalScrollbar->offsetDidChange();
    if (m_verticalScrollbar)
        m_verticalScrollbar->offsetDidChange();

    m_inUpdateScrollbars = false;
}

const int panIconSizeLength = 16;

IntRect ScrollView::rectToCopyOnScroll() const
{
    IntRect scrollViewRect = convertToContainingWindow(IntRect(0, 0, visibleWidth(), visibleHeight()));
    if (hasOverlayScrollbars()) {
        int verticalScrollbarWidth = (verticalScrollbar() && !hasLayerForVerticalScrollbar()) ? verticalScrollbar()->width() : 0;
        int horizontalScrollbarHeight = (horizontalScrollbar() && !hasLayerForHorizontalScrollbar()) ? horizontalScrollbar()->height() : 0;
        
        scrollViewRect.setWidth(scrollViewRect.width() - verticalScrollbarWidth);
        scrollViewRect.setHeight(scrollViewRect.height() - horizontalScrollbarHeight);
    }
    return scrollViewRect;
}

void ScrollView::scrollContents(const IntSize& scrollDelta)
{
    if (!hostWindow())
        return;

    // Since scrolling is double buffered, we will be blitting the scroll view's intersection
    // with the clip rect every time to keep it smooth.
    IntRect clipRect = windowClipRect();
    IntRect scrollViewRect = rectToCopyOnScroll();    
    IntRect updateRect = clipRect;
    updateRect.intersect(scrollViewRect);

    // Invalidate the window (not the backing store).
    hostWindow()->invalidateWindow(updateRect, false /*immediate*/);

    if (m_drawPanScrollIcon) {
        // FIXME: the pan icon is broken when accelerated compositing is on, since it will draw under the compositing layers.
        // https://bugs.webkit.org/show_bug.cgi?id=47837
        int panIconDirtySquareSizeLength = 2 * (panIconSizeLength + max(abs(scrollDelta.width()), abs(scrollDelta.height()))); // We only want to repaint what's necessary
        IntPoint panIconDirtySquareLocation = IntPoint(m_panScrollIconPoint.x() - (panIconDirtySquareSizeLength / 2), m_panScrollIconPoint.y() - (panIconDirtySquareSizeLength / 2));
        IntRect panScrollIconDirtyRect = IntRect(panIconDirtySquareLocation , IntSize(panIconDirtySquareSizeLength, panIconDirtySquareSizeLength));
        panScrollIconDirtyRect.intersect(clipRect);
        hostWindow()->invalidateContentsAndWindow(panScrollIconDirtyRect, false /*immediate*/);
    }

    if (canBlitOnScroll()) { // The main frame can just blit the WebView window
        // FIXME: Find a way to scroll subframes with this faster path
        if (!scrollContentsFastPath(-scrollDelta, scrollViewRect, clipRect))
            scrollContentsSlowPath(updateRect);
    } else { 
       // We need to go ahead and repaint the entire backing store.  Do it now before moving the
       // windowed plugins.
       scrollContentsSlowPath(updateRect);
    }

    // Invalidate the overhang areas if they are visible.
    IntRect horizontalOverhangRect;
    IntRect verticalOverhangRect;
    calculateOverhangAreasForPainting(horizontalOverhangRect, verticalOverhangRect);
    if (!horizontalOverhangRect.isEmpty())
        hostWindow()->invalidateContentsAndWindow(horizontalOverhangRect, false /*immediate*/);
    if (!verticalOverhangRect.isEmpty())
        hostWindow()->invalidateContentsAndWindow(verticalOverhangRect, false /*immediate*/);

    // This call will move children with native widgets (plugins) and invalidate them as well.
    frameRectsChanged();

    // Now blit the backingstore into the window which should be very fast.
    hostWindow()->invalidateWindow(IntRect(), true);
}

bool ScrollView::scrollContentsFastPath(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect)
{
    hostWindow()->scroll(scrollDelta, rectToScroll, clipRect);
    return true;
}

void ScrollView::scrollContentsSlowPath(const IntRect& updateRect)
{
    hostWindow()->invalidateContentsForSlowScroll(updateRect, false);
}

IntPoint ScrollView::windowToContents(const IntPoint& windowPoint) const
{
    IntPoint viewPoint = convertFromContainingWindow(windowPoint);
    return viewPoint + scrollOffset();
}

IntPoint ScrollView::contentsToWindow(const IntPoint& contentsPoint) const
{
    IntPoint viewPoint = contentsPoint - scrollOffset();
    return convertToContainingWindow(viewPoint);  
}

IntRect ScrollView::windowToContents(const IntRect& windowRect) const
{
    IntRect viewRect = convertFromContainingWindow(windowRect);
    viewRect.move(scrollOffset());
    return viewRect;
}

IntRect ScrollView::contentsToWindow(const IntRect& contentsRect) const
{
    IntRect viewRect = contentsRect;
    viewRect.move(-scrollOffset());
    return convertToContainingWindow(viewRect);
}

IntRect ScrollView::contentsToScreen(const IntRect& rect) const
{
    if (platformWidget())
        return platformContentsToScreen(rect);
    if (!hostWindow())
        return IntRect();
    return hostWindow()->windowToScreen(contentsToWindow(rect));
}

IntPoint ScrollView::screenToContents(const IntPoint& point) const
{
    if (platformWidget())
        return platformScreenToContents(point);
    if (!hostWindow())
        return IntPoint();
    return windowToContents(hostWindow()->screenToWindow(point));
}

bool ScrollView::containsScrollbarsAvoidingResizer() const
{
    return !m_scrollbarsAvoidingResizer;
}

void ScrollView::adjustScrollbarsAvoidingResizerCount(int overlapDelta)
{
    int oldCount = m_scrollbarsAvoidingResizer;
    m_scrollbarsAvoidingResizer += overlapDelta;
    if (parent())
        parent()->adjustScrollbarsAvoidingResizerCount(overlapDelta);
    else if (!scrollbarsSuppressed()) {
        // If we went from n to 0 or from 0 to n and we're the outermost view,
        // we need to invalidate the windowResizerRect(), since it will now need to paint
        // differently.
        if ((oldCount > 0 && m_scrollbarsAvoidingResizer == 0) ||
            (oldCount == 0 && m_scrollbarsAvoidingResizer > 0))
            invalidateRect(windowResizerRect());
    }
}

void ScrollView::setParent(ScrollView* parentView)
{
    if (parentView == parent())
        return;

    if (m_scrollbarsAvoidingResizer && parent())
        parent()->adjustScrollbarsAvoidingResizerCount(-m_scrollbarsAvoidingResizer);

    Widget::setParent(parentView);

    if (m_scrollbarsAvoidingResizer && parent())
        parent()->adjustScrollbarsAvoidingResizerCount(m_scrollbarsAvoidingResizer);
}

void ScrollView::setScrollbarsSuppressed(bool suppressed, bool repaintOnUnsuppress)
{
    if (suppressed == m_scrollbarsSuppressed)
        return;

    m_scrollbarsSuppressed = suppressed;

    if (platformWidget())
        platformSetScrollbarsSuppressed(repaintOnUnsuppress);
    else if (repaintOnUnsuppress && !suppressed) {
        if (m_horizontalScrollbar)
            m_horizontalScrollbar->invalidate();
        if (m_verticalScrollbar)
            m_verticalScrollbar->invalidate();

        // Invalidate the scroll corner too on unsuppress.
        invalidateRect(scrollCornerRect());
    }
}

Scrollbar* ScrollView::scrollbarAtPoint(const IntPoint& windowPoint)
{
    if (platformWidget())
        return 0;

    IntPoint viewPoint = convertFromContainingWindow(windowPoint);
    if (m_horizontalScrollbar && m_horizontalScrollbar->frameRect().contains(viewPoint))
        return m_horizontalScrollbar.get();
    if (m_verticalScrollbar && m_verticalScrollbar->frameRect().contains(viewPoint))
        return m_verticalScrollbar.get();
    return 0;
}

void ScrollView::wheelEvent(PlatformWheelEvent& e)
{
    // We don't allow mouse wheeling to happen in a ScrollView that has had its scrollbars explicitly disabled.
#if PLATFORM(WX)
    if (!canHaveScrollbars()) {
#else
    if (!canHaveScrollbars() || platformWidget()) {
#endif
        return;
    }

    ScrollableArea::handleWheelEvent(e);
}

#if ENABLE(GESTURE_EVENTS)
void ScrollView::gestureEvent(const PlatformGestureEvent& gestureEvent)
{
    if (platformWidget())
        return;

    ScrollableArea::handleGestureEvent(gestureEvent);
}
#endif

void ScrollView::setFrameRect(const IntRect& newRect)
{
    IntRect oldRect = frameRect();
    
    if (newRect == oldRect)
        return;

    Widget::setFrameRect(newRect);

    frameRectsChanged();
}

void ScrollView::setBoundsSize(const IntSize& newSize)
{
    if (newSize == m_boundsSize)
        return;

    Widget::setBoundsSize(newSize);
    m_boundsSize = newSize;

    if (platformWidget())
        return;

    updateScrollbars(m_scrollOffset);
    if (!m_useFixedLayout)
        contentsResized();

    positionScrollbarLayers();
}

void ScrollView::setInitialBoundsSize(const IntSize& newSize)
{
    ASSERT(m_boundsSize.isZero());
    m_boundsSize = newSize;
}

void ScrollView::frameRectsChanged()
{
    if (platformWidget())
        return;

    HashSet<RefPtr<Widget> >::const_iterator end = m_children.end();
    for (HashSet<RefPtr<Widget> >::const_iterator current = m_children.begin(); current != end; ++current)
        (*current)->frameRectsChanged();
}

#if USE(ACCELERATED_COMPOSITING)
static void positionScrollbarLayer(GraphicsLayer* graphicsLayer, Scrollbar* scrollbar)
{
    if (!graphicsLayer || !scrollbar)
        return;
    graphicsLayer->setDrawsContent(true);
    IntRect scrollbarRect = scrollbar->frameRect();
    graphicsLayer->setPosition(scrollbarRect.location());
    if (scrollbarRect.size() != graphicsLayer->size())
        graphicsLayer->setNeedsDisplay();
    graphicsLayer->setSize(scrollbarRect.size());
}

static void positionScrollCornerLayer(GraphicsLayer* graphicsLayer, const IntRect& cornerRect)
{
    if (!graphicsLayer)
        return;
    graphicsLayer->setDrawsContent(!cornerRect.isEmpty());
    graphicsLayer->setPosition(cornerRect.location());
    if (cornerRect.size() != graphicsLayer->size())
        graphicsLayer->setNeedsDisplay();
    graphicsLayer->setSize(cornerRect.size());
}
#endif


void ScrollView::positionScrollbarLayers()
{
#if USE(ACCELERATED_COMPOSITING)
    positionScrollbarLayer(layerForHorizontalScrollbar(), horizontalScrollbar());
    positionScrollbarLayer(layerForVerticalScrollbar(), verticalScrollbar());
    positionScrollCornerLayer(layerForScrollCorner(), scrollCornerRect());
#endif
}

void ScrollView::repaintContentRectangle(const IntRect& rect, bool now)
{
    IntRect paintRect = rect;
    if (clipsRepaints() && !paintsEntireContents())
        paintRect.intersect(visibleContentRect());
    if (paintRect.isEmpty())
        return;

    if (platformWidget()) {
        platformRepaintContentRectangle(paintRect, now);
        return;
    }

    if (hostWindow())
        hostWindow()->invalidateContentsAndWindow(contentsToWindow(paintRect), now /*immediate*/);
}

IntRect ScrollView::scrollCornerRect() const
{
    IntRect cornerRect;

    if (hasOverlayScrollbars())
        return cornerRect;

    if (m_horizontalScrollbar && m_boundsSize.width() - m_horizontalScrollbar->width() > 0) {
        cornerRect.unite(IntRect(m_horizontalScrollbar->width(),
                                 m_boundsSize.height() - m_horizontalScrollbar->height(),
                                 m_boundsSize.width() - m_horizontalScrollbar->width(),
                                 m_horizontalScrollbar->height()));
    }

    if (m_verticalScrollbar && m_boundsSize.height() - m_verticalScrollbar->height() > 0) {
        cornerRect.unite(IntRect(m_boundsSize.width() - m_verticalScrollbar->width(),
                                 m_verticalScrollbar->height(),
                                 m_verticalScrollbar->width(),
                                 m_boundsSize.height() - m_verticalScrollbar->height()));
    }
    
    return cornerRect;
}

bool ScrollView::isScrollCornerVisible() const
{
    return !scrollCornerRect().isEmpty();
}

void ScrollView::updateScrollCorner()
{
}

void ScrollView::paintScrollCorner(GraphicsContext* context, const IntRect& cornerRect)
{
    ScrollbarTheme::nativeTheme()->paintScrollCorner(this, context, cornerRect);
}

void ScrollView::invalidateScrollCornerRect(const IntRect& rect)
{
    invalidateRect(rect);
}

void ScrollView::paintScrollbars(GraphicsContext* context, const IntRect& rect)
{
    if (m_horizontalScrollbar
#if USE(ACCELERATED_COMPOSITING)
        && !layerForHorizontalScrollbar()
#endif
                                      )
        m_horizontalScrollbar->paint(context, rect);
    if (m_verticalScrollbar
#if USE(ACCELERATED_COMPOSITING)
        && !layerForVerticalScrollbar()
#endif
                                    )
        m_verticalScrollbar->paint(context, rect);

#if USE(ACCELERATED_COMPOSITING)
    if (layerForScrollCorner())
        return;
#endif
    paintScrollCorner(context, scrollCornerRect());
}

void ScrollView::paintPanScrollIcon(GraphicsContext* context)
{
    static Image* panScrollIcon = Image::loadPlatformResource("panIcon").releaseRef();
    context->drawImage(panScrollIcon, ColorSpaceDeviceRGB, m_panScrollIconPoint);
}

void ScrollView::paint(GraphicsContext* context, const IntRect& rect)
{
    if (platformWidget()) {
        Widget::paint(context, rect);
        return;
    }

    if (context->paintingDisabled() && !context->updatingControlTints())
        return;

    notifyPageThatContentAreaWillPaint();

    // If we encounter any overlay scrollbars as we paint, this will be set to true.
    m_containsScrollableAreaWithOverlayScrollbars = false;

    IntRect clipRect = frameRect();
    if (verticalScrollbar() && !verticalScrollbar()->isOverlayScrollbar())
        clipRect.setWidth(clipRect.width() - verticalScrollbar()->width());
    if (horizontalScrollbar() && !horizontalScrollbar()->isOverlayScrollbar())
        clipRect.setHeight(clipRect.height() - horizontalScrollbar()->height());

    IntRect documentDirtyRect = rect;
    documentDirtyRect.intersect(clipRect);

    if (!documentDirtyRect.isEmpty()) {
        GraphicsContextStateSaver stateSaver(*context);

        context->translate(x(), y());
        documentDirtyRect.move(-x(), -y());

        if (!paintsEntireContents()) {
            context->translate(-scrollX(), -scrollY());
            documentDirtyRect.move(scrollX(), scrollY());

            context->clip(visibleContentRect());
        }

        paintContents(context, documentDirtyRect);
    }

    IntRect horizontalOverhangRect;
    IntRect verticalOverhangRect;
    calculateOverhangAreasForPainting(horizontalOverhangRect, verticalOverhangRect);

    if (rect.intersects(horizontalOverhangRect) || rect.intersects(verticalOverhangRect))
        paintOverhangAreas(context, horizontalOverhangRect, verticalOverhangRect, rect);

    // Now paint the scrollbars.
    if (!m_scrollbarsSuppressed && (m_horizontalScrollbar || m_verticalScrollbar)) {
        GraphicsContextStateSaver stateSaver(*context);
        IntRect scrollViewDirtyRect = rect;
        scrollViewDirtyRect.intersect(frameRect());
        context->translate(x(), y());
        scrollViewDirtyRect.move(-x(), -y());

        paintScrollbars(context, scrollViewDirtyRect);
    }

    // Paint the panScroll Icon
    if (m_drawPanScrollIcon)
        paintPanScrollIcon(context);
}

void ScrollView::calculateOverhangAreasForPainting(IntRect& horizontalOverhangRect, IntRect& verticalOverhangRect)
{
    int verticalScrollbarWidth = (verticalScrollbar() && !verticalScrollbar()->isOverlayScrollbar())
        ? verticalScrollbar()->width() : 0;
    int horizontalScrollbarHeight = (horizontalScrollbar() && !horizontalScrollbar()->isOverlayScrollbar())
        ? horizontalScrollbar()->height() : 0;

    int physicalScrollY = scrollPosition().y() + m_scrollOrigin.y();
    if (physicalScrollY < 0) {
        horizontalOverhangRect = frameRect();
        horizontalOverhangRect.setHeight(-physicalScrollY);
    } else if (physicalScrollY > contentsHeight() - visibleContentRect().height()) {
        int height = physicalScrollY - (contentsHeight() - visibleContentRect().height());
        horizontalOverhangRect = frameRect();
        horizontalOverhangRect.setY(frameRect().maxY() - height - horizontalScrollbarHeight);
        horizontalOverhangRect.setHeight(height);
    }

    int physicalScrollX = scrollPosition().x() + m_scrollOrigin.x();
    if (physicalScrollX < 0) {
        verticalOverhangRect.setWidth(-physicalScrollX);
        verticalOverhangRect.setHeight(frameRect().height() - horizontalOverhangRect.height());
        verticalOverhangRect.setX(frameRect().x());
        if (horizontalOverhangRect.y() == frameRect().y())
            verticalOverhangRect.setY(frameRect().y() + horizontalOverhangRect.height());
        else
            verticalOverhangRect.setY(frameRect().y());
    } else if (physicalScrollX > contentsWidth() - visibleContentRect().width()) {
        int width = physicalScrollX - (contentsWidth() - visibleContentRect().width());
        verticalOverhangRect.setWidth(width);
        verticalOverhangRect.setHeight(frameRect().height() - horizontalOverhangRect.height());
        verticalOverhangRect.setX(frameRect().maxX() - width - verticalScrollbarWidth);
        if (horizontalOverhangRect.y() == frameRect().y())
            verticalOverhangRect.setY(frameRect().y() + horizontalOverhangRect.height());
        else
            verticalOverhangRect.setY(frameRect().y());
    }
}

void ScrollView::paintOverhangAreas(GraphicsContext* context, const IntRect& horizontalOverhangRect, const IntRect& verticalOverhangRect, const IntRect&)
{
    // FIXME: This should be checking the dirty rect.

    context->setFillColor(Color::white, ColorSpaceDeviceRGB);
    if (!horizontalOverhangRect.isEmpty())
        context->fillRect(horizontalOverhangRect);

    context->setFillColor(Color::white, ColorSpaceDeviceRGB);
    if (!verticalOverhangRect.isEmpty())
        context->fillRect(verticalOverhangRect);
}

bool ScrollView::isPointInScrollbarCorner(const IntPoint& windowPoint)
{
    if (!scrollbarCornerPresent())
        return false;

    IntPoint viewPoint = convertFromContainingWindow(windowPoint);

    if (m_horizontalScrollbar) {
        int horizontalScrollbarYMin = m_horizontalScrollbar->frameRect().y();
        int horizontalScrollbarYMax = m_horizontalScrollbar->frameRect().y() + m_horizontalScrollbar->frameRect().height();
        int horizontalScrollbarXMin = m_horizontalScrollbar->frameRect().x() + m_horizontalScrollbar->frameRect().width();

        return viewPoint.y() > horizontalScrollbarYMin && viewPoint.y() < horizontalScrollbarYMax && viewPoint.x() > horizontalScrollbarXMin;
    }

    int verticalScrollbarXMin = m_verticalScrollbar->frameRect().x();
    int verticalScrollbarXMax = m_verticalScrollbar->frameRect().x() + m_verticalScrollbar->frameRect().width();
    int verticalScrollbarYMin = m_verticalScrollbar->frameRect().y() + m_verticalScrollbar->frameRect().height();
    
    return viewPoint.x() > verticalScrollbarXMin && viewPoint.x() < verticalScrollbarXMax && viewPoint.y() > verticalScrollbarYMin;
}

bool ScrollView::scrollbarCornerPresent() const
{
    return (m_horizontalScrollbar && m_boundsSize.width() - m_horizontalScrollbar->width() > 0) ||
           (m_verticalScrollbar && m_boundsSize.height() - m_verticalScrollbar->height() > 0);
}

IntRect ScrollView::convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntRect& localRect) const
{
    // Scrollbars won't be transformed within us
    IntRect newRect = localRect;
    newRect.move(scrollbar->x(), scrollbar->y());
    return newRect;
}

IntRect ScrollView::convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntRect& parentRect) const
{
    IntRect newRect = parentRect;
    // Scrollbars won't be transformed within us
    newRect.move(-scrollbar->x(), -scrollbar->y());
    return newRect;
}

// FIXME: test these on windows
IntPoint ScrollView::convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntPoint& localPoint) const
{
    // Scrollbars won't be transformed within us
    IntPoint newPoint = localPoint;
    newPoint.move(scrollbar->x(), scrollbar->y());
    return newPoint;
}

IntPoint ScrollView::convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntPoint& parentPoint) const
{
    IntPoint newPoint = parentPoint;
    // Scrollbars won't be transformed within us
    newPoint.move(-scrollbar->x(), -scrollbar->y());
    return newPoint;
}

void ScrollView::setParentVisible(bool visible)
{
    if (isParentVisible() == visible)
        return;
    
    Widget::setParentVisible(visible);

    if (!isSelfVisible())
        return;
        
    HashSet<RefPtr<Widget> >::iterator end = m_children.end();
    for (HashSet<RefPtr<Widget> >::iterator it = m_children.begin(); it != end; ++it)
        (*it)->setParentVisible(visible);
}

void ScrollView::show()
{
    if (!isSelfVisible()) {
        setSelfVisible(true);
        if (isParentVisible()) {
            HashSet<RefPtr<Widget> >::iterator end = m_children.end();
            for (HashSet<RefPtr<Widget> >::iterator it = m_children.begin(); it != end; ++it)
                (*it)->setParentVisible(true);
        }
    }

    Widget::show();
}

void ScrollView::hide()
{
    if (isSelfVisible()) {
        if (isParentVisible()) {
            HashSet<RefPtr<Widget> >::iterator end = m_children.end();
            for (HashSet<RefPtr<Widget> >::iterator it = m_children.begin(); it != end; ++it)
                (*it)->setParentVisible(false);
        }
        setSelfVisible(false);
    }

    Widget::hide();
}

bool ScrollView::isOffscreen() const
{
    if (platformWidget())
        return platformIsOffscreen();
    
    if (!isVisible())
        return true;
    
    // FIXME: Add a HostWindow::isOffscreen method here.  Since only Mac implements this method
    // currently, we can add the method when the other platforms decide to implement this concept.
    return false;
}


void ScrollView::addPanScrollIcon(const IntPoint& iconPosition)
{
    if (!hostWindow())
        return;
    m_drawPanScrollIcon = true;    
    m_panScrollIconPoint = IntPoint(iconPosition.x() - panIconSizeLength / 2 , iconPosition.y() - panIconSizeLength / 2) ;
    hostWindow()->invalidateContentsAndWindow(IntRect(m_panScrollIconPoint, IntSize(panIconSizeLength, panIconSizeLength)), true /*immediate*/);
}

void ScrollView::removePanScrollIcon()
{
    if (!hostWindow())
        return;
    m_drawPanScrollIcon = false; 
    hostWindow()->invalidateContentsAndWindow(IntRect(m_panScrollIconPoint, IntSize(panIconSizeLength, panIconSizeLength)), true /*immediate*/);
}

void ScrollView::setScrollOrigin(const IntPoint& origin, bool updatePositionAtAll, bool updatePositionSynchronously)
{
    if (m_scrollOrigin == origin)
        return;

    m_scrollOrigin = origin;

    if (platformWidget()) {
        platformSetScrollOrigin(origin, updatePositionAtAll, updatePositionSynchronously);
        return;
    }
    
    // Update if the scroll origin changes, since our position will be different if the content size did not change.
    if (updatePositionAtAll && updatePositionSynchronously)
        updateScrollbars(scrollOffset());
}

#if !PLATFORM(WX) && !PLATFORM(GTK) && !PLATFORM(EFL)

void ScrollView::platformInit()
{
}

void ScrollView::platformDestroy()
{
}

#endif

#if !PLATFORM(WX) && !PLATFORM(QT) && !PLATFORM(MAC)

void ScrollView::platformAddChild(Widget*)
{
}

void ScrollView::platformRemoveChild(Widget*)
{
}

#endif

#if !PLATFORM(MAC)

void ScrollView::platformSetScrollbarsSuppressed(bool)
{
}

void ScrollView::platformSetScrollOrigin(const IntPoint&, bool updatePositionAtAll, bool updatePositionSynchronously)
{
}

#endif

#if !PLATFORM(MAC) && !PLATFORM(WX)

void ScrollView::platformSetScrollbarModes()
{
}

void ScrollView::platformScrollbarModes(ScrollbarMode& horizontal, ScrollbarMode& vertical) const
{
    horizontal = ScrollbarAuto;
    vertical = ScrollbarAuto;
}

void ScrollView::platformSetCanBlitOnScroll(bool)
{
}

bool ScrollView::platformCanBlitOnScroll() const
{
    return false;
}

IntRect ScrollView::platformVisibleContentRect(bool) const
{
    return IntRect();
}

IntSize ScrollView::platformContentsSize() const
{
    return IntSize();
}

void ScrollView::platformSetContentsSize()
{
}

IntRect ScrollView::platformContentsToScreen(const IntRect& rect) const
{
    return rect;
}

IntPoint ScrollView::platformScreenToContents(const IntPoint& point) const
{
    return point;
}

void ScrollView::platformSetScrollPosition(const IntPoint&)
{
}

bool ScrollView::platformScroll(ScrollDirection, ScrollGranularity)
{
    return true;
}

void ScrollView::platformRepaintContentRectangle(const IntRect&, bool /*now*/)
{
}

bool ScrollView::platformIsOffscreen() const
{
    return false;
}

#endif

}
