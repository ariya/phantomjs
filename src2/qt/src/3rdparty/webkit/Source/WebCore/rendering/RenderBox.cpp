/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2005, 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "RenderBox.h"

#include "CachedImage.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "htmlediting.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "ImageBuffer.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderArena.h"
#include "RenderFlexibleBox.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderTableCell.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "ScrollbarTheme.h"
#include "TransformState.h"
#include <algorithm>
#include <math.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

// Used by flexible boxes when flexing this element.
typedef WTF::HashMap<const RenderBox*, int> OverrideSizeMap;
static OverrideSizeMap* gOverrideSizeMap = 0;

bool RenderBox::s_hadOverflowClip = false;

RenderBox::RenderBox(Node* node)
    : RenderBoxModelObject(node)
    , m_marginLeft(0)
    , m_marginRight(0)
    , m_marginTop(0)
    , m_marginBottom(0)
    , m_minPreferredLogicalWidth(-1)
    , m_maxPreferredLogicalWidth(-1)
    , m_inlineBoxWrapper(0)
{
    setIsBox();
}

RenderBox::~RenderBox()
{
}

int RenderBox::marginBefore() const
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        return m_marginTop;
    case BottomToTopWritingMode:
        return m_marginBottom;
    case LeftToRightWritingMode:
        return m_marginLeft;
    case RightToLeftWritingMode:
        return m_marginRight;
    }
    ASSERT_NOT_REACHED();
    return m_marginTop;
}

int RenderBox::marginAfter() const
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        return m_marginBottom;
    case BottomToTopWritingMode:
        return m_marginTop;
    case LeftToRightWritingMode:
        return m_marginRight;
    case RightToLeftWritingMode:
        return m_marginLeft;
    }
    ASSERT_NOT_REACHED();
    return m_marginBottom;
}

int RenderBox::marginStart() const
{
    if (isHorizontalWritingMode())
        return style()->isLeftToRightDirection() ? m_marginLeft : m_marginRight;
    return style()->isLeftToRightDirection() ? m_marginTop : m_marginBottom;
}

int RenderBox::marginEnd() const
{
    if (isHorizontalWritingMode())
        return style()->isLeftToRightDirection() ? m_marginRight : m_marginLeft;
    return style()->isLeftToRightDirection() ? m_marginBottom : m_marginTop;
}

void RenderBox::setMarginStart(int margin)
{
    if (isHorizontalWritingMode()) {
        if (style()->isLeftToRightDirection())
            m_marginLeft = margin;
        else
            m_marginRight = margin;
    } else {
        if (style()->isLeftToRightDirection())
            m_marginTop = margin;
        else
            m_marginBottom = margin;
    }
}

void RenderBox::setMarginEnd(int margin)
{
    if (isHorizontalWritingMode()) {
        if (style()->isLeftToRightDirection())
            m_marginRight = margin;
        else
            m_marginLeft = margin;
    } else {
        if (style()->isLeftToRightDirection())
            m_marginBottom = margin;
        else
            m_marginTop = margin;
    }
}

void RenderBox::setMarginBefore(int margin)
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        m_marginTop = margin;
        break;
    case BottomToTopWritingMode:
        m_marginBottom = margin;
        break;
    case LeftToRightWritingMode:
        m_marginLeft = margin;
        break;
    case RightToLeftWritingMode:
        m_marginRight = margin;
        break;
    }
}

void RenderBox::setMarginAfter(int margin)
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        m_marginBottom = margin;
        break;
    case BottomToTopWritingMode:
        m_marginTop = margin;
        break;
    case LeftToRightWritingMode:
        m_marginRight = margin;
        break;
    case RightToLeftWritingMode:
        m_marginLeft = margin;
        break;
    }
}

void RenderBox::destroy()
{
    // A lot of the code in this function is just pasted into
    // RenderWidget::destroy. If anything in this function changes,
    // be sure to fix RenderWidget::destroy() as well.
    if (hasOverrideSize())
        gOverrideSizeMap->remove(this);

    if (style() && (style()->logicalHeight().isPercent() || style()->logicalMinHeight().isPercent() || style()->logicalMaxHeight().isPercent()))
        RenderBlock::removePercentHeightDescendant(this);

    // If this renderer is owning renderer for the frameview's custom scrollbars,
    // we need to clear it from the scrollbar. See webkit bug 64737.
    if (style() && style()->hasPseudoStyle(SCROLLBAR) && frame() && frame()->view())
        frame()->view()->clearOwningRendererForCustomScrollbars(this);

    RenderBoxModelObject::destroy();
}

void RenderBox::removeFloatingOrPositionedChildFromBlockLists()
{
    ASSERT(isFloatingOrPositioned());

    if (documentBeingDestroyed())
        return;

    if (isFloating()) {
        RenderBlock* parentBlock = 0;
        for (RenderObject* curr = parent(); curr && !curr->isRenderView(); curr = curr->parent()) {
            if (curr->isRenderBlock()) {
                RenderBlock* currBlock = toRenderBlock(curr);
                if (!parentBlock || currBlock->containsFloat(this))
                    parentBlock = currBlock;
            }
        }

        if (parentBlock) {
            RenderObject* parent = parentBlock->parent();
            if (parent && parent->isFlexibleBox())
                parentBlock = toRenderBlock(parent);

            parentBlock->markAllDescendantsWithFloatsForLayout(this, false);
        }
    }

    if (isPositioned()) {
        for (RenderObject* curr = parent(); curr; curr = curr->parent()) {
            if (curr->isRenderBlock())
                toRenderBlock(curr)->removePositionedObject(this);
        }
    }
}

void RenderBox::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    s_hadOverflowClip = hasOverflowClip();

    if (style()) {
        // The background of the root element or the body element could propagate up to
        // the canvas.  Just dirty the entire canvas when our style changes substantially.
        if (diff >= StyleDifferenceRepaint && node() &&
                (node()->hasTagName(htmlTag) || node()->hasTagName(bodyTag)))
            view()->repaint();
        
        // When a layout hint happens and an object's position style changes, we have to do a layout
        // to dirty the render tree using the old position value now.
        if (diff == StyleDifferenceLayout && parent() && style()->position() != newStyle->position()) {
            markContainingBlocksForLayout();
            if (style()->position() == StaticPosition)
                repaint();
            else if (newStyle->position() == AbsolutePosition || newStyle->position() == FixedPosition)
                parent()->setChildNeedsLayout(true);
            if (isFloating() && !isPositioned() && (newStyle->position() == AbsolutePosition || newStyle->position() == FixedPosition))
                removeFloatingOrPositionedChildFromBlockLists();
        }
    } else if (newStyle && isBody())
        view()->repaint();

    if (FrameView *frameView = view()->frameView()) {
        bool newStyleIsFixed = newStyle && newStyle->position() == FixedPosition;
        bool oldStyleIsFixed = style() && style()->position() == FixedPosition;
        if (newStyleIsFixed != oldStyleIsFixed) {
            if (newStyleIsFixed)
                frameView->addFixedObject();
            else
                frameView->removeFixedObject();
        }
    }

    RenderBoxModelObject::styleWillChange(diff, newStyle);
}

void RenderBox::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBoxModelObject::styleDidChange(diff, oldStyle);

    if (needsLayout() && oldStyle) {
        if (oldStyle && (oldStyle->logicalHeight().isPercent() || oldStyle->logicalMinHeight().isPercent() || oldStyle->logicalMaxHeight().isPercent()))
            RenderBlock::removePercentHeightDescendant(this);

        // Normally we can do optimized positioning layout for absolute/fixed positioned objects. There is one special case, however, which is
        // when the positioned object's margin-before is changed. In this case the parent has to get a layout in order to run margin collapsing
        // to determine the new static position.
        if (isPositioned() && style()->hasStaticBlockPosition(isHorizontalWritingMode()) && oldStyle->marginBefore() != style()->marginBefore()
            && parent() && !parent()->normalChildNeedsLayout())
            parent()->setChildNeedsLayout(true);
    }

    // If our zoom factor changes and we have a defined scrollLeft/Top, we need to adjust that value into the
    // new zoomed coordinate space.
    if (hasOverflowClip() && oldStyle && style() && oldStyle->effectiveZoom() != style()->effectiveZoom()) {
        if (int left = layer()->scrollXOffset()) {
            left = (left / oldStyle->effectiveZoom()) * style()->effectiveZoom();
            layer()->scrollToXOffset(left);
        }
        if (int top = layer()->scrollYOffset()) {
            top = (top / oldStyle->effectiveZoom()) * style()->effectiveZoom();
            layer()->scrollToYOffset(top);
        }
    }

    bool isBodyRenderer = isBody();
    bool isRootRenderer = isRoot();

    // Set the text color if we're the body.
    if (isBodyRenderer)
        document()->setTextColor(style()->visitedDependentColor(CSSPropertyColor));

    if (isRootRenderer || isBodyRenderer) {
        // Propagate the new writing mode and direction up to the RenderView.
        RenderView* viewRenderer = view();
        RenderStyle* viewStyle = viewRenderer->style();
        if (viewStyle->direction() != style()->direction() && (isRootRenderer || !document()->directionSetOnDocumentElement())) {
            viewStyle->setDirection(style()->direction());
            if (isBodyRenderer)
                document()->documentElement()->renderer()->style()->setDirection(style()->direction());
            setNeedsLayoutAndPrefWidthsRecalc();
        }

        if (viewStyle->writingMode() != style()->writingMode() && (isRootRenderer || !document()->writingModeSetOnDocumentElement())) {
            viewStyle->setWritingMode(style()->writingMode());
            viewRenderer->setHorizontalWritingMode(style()->isHorizontalWritingMode());
            if (isBodyRenderer) {
                document()->documentElement()->renderer()->style()->setWritingMode(style()->writingMode());
                document()->documentElement()->renderer()->setHorizontalWritingMode(style()->isHorizontalWritingMode());
            }
            setNeedsLayoutAndPrefWidthsRecalc();
        }
    }
}

void RenderBox::updateBoxModelInfoFromStyle()
{
    RenderBoxModelObject::updateBoxModelInfoFromStyle();

    bool isRootObject = isRoot();
    bool isViewObject = isRenderView();

    // The root and the RenderView always paint their backgrounds/borders.
    if (isRootObject || isViewObject)
        setHasBoxDecorations(true);

    setPositioned(style()->position() == AbsolutePosition || style()->position() == FixedPosition);
    setFloating(!isPositioned() && style()->isFloating());

    // We also handle <body> and <html>, whose overflow applies to the viewport.
    if (style()->overflowX() != OVISIBLE && !isRootObject && (isRenderBlock() || isTableRow() || isTableSection())) {
        bool boxHasOverflowClip = true;
        if (isBody()) {
            // Overflow on the body can propagate to the viewport under the following conditions.
            // (1) The root element is <html>.
            // (2) We are the primary <body> (can be checked by looking at document.body).
            // (3) The root element has visible overflow.
            if (document()->documentElement()->hasTagName(htmlTag) &&
                document()->body() == node() &&
                document()->documentElement()->renderer()->style()->overflowX() == OVISIBLE)
                boxHasOverflowClip = false;
        }
        
        // Check for overflow clip.
        // It's sufficient to just check one direction, since it's illegal to have visible on only one overflow value.
        if (boxHasOverflowClip) {
            if (!s_hadOverflowClip)
                // Erase the overflow
                repaint();
            setHasOverflowClip();
        }
    }

    setHasTransform(style()->hasTransformRelatedProperty());
    setHasReflection(style()->boxReflect());
}

void RenderBox::layout()
{
    ASSERT(needsLayout());

    RenderObject* child = firstChild();
    if (!child) {
        setNeedsLayout(false);
        return;
    }

    LayoutStateMaintainer statePusher(view(), this, IntSize(x(), y()), style()->isFlippedBlocksWritingMode());
    while (child) {
        child->layoutIfNeeded();
        ASSERT(!child->needsLayout());
        child = child->nextSibling();
    }
    statePusher.pop();
    setNeedsLayout(false);
}

// More IE extensions.  clientWidth and clientHeight represent the interior of an object
// excluding border and scrollbar.
int RenderBox::clientWidth() const
{
    return width() - borderLeft() - borderRight() - verticalScrollbarWidth();
}

int RenderBox::clientHeight() const
{
    return height() - borderTop() - borderBottom() - horizontalScrollbarHeight();
}

int RenderBox::scrollWidth() const
{
    if (hasOverflowClip())
        return layer()->scrollWidth();
    // For objects with visible overflow, this matches IE.
    // FIXME: Need to work right with writing modes.
    if (style()->isLeftToRightDirection())
        return max(clientWidth(), maxXLayoutOverflow() - borderLeft());
    return clientWidth() - min(0, minXLayoutOverflow() - borderLeft());
}

int RenderBox::scrollHeight() const
{
    if (hasOverflowClip())
        return layer()->scrollHeight();
    // For objects with visible overflow, this matches IE.
    // FIXME: Need to work right with writing modes.
    return max(clientHeight(), maxYLayoutOverflow() - borderTop());
}

int RenderBox::scrollLeft() const
{
    return hasOverflowClip() ? layer()->scrollXOffset() : 0;
}

int RenderBox::scrollTop() const
{
    return hasOverflowClip() ? layer()->scrollYOffset() : 0;
}

void RenderBox::setScrollLeft(int newLeft)
{
    if (hasOverflowClip())
        layer()->scrollToXOffset(newLeft);
}

void RenderBox::setScrollTop(int newTop)
{
    if (hasOverflowClip())
        layer()->scrollToYOffset(newTop);
}

void RenderBox::absoluteRects(Vector<IntRect>& rects, int tx, int ty)
{
    rects.append(IntRect(tx, ty, width(), height()));
}

void RenderBox::absoluteQuads(Vector<FloatQuad>& quads)
{
    quads.append(localToAbsoluteQuad(FloatRect(0, 0, width(), height())));
}

void RenderBox::updateLayerTransform()
{
    // Transform-origin depends on box size, so we need to update the layer transform after layout.
    if (hasLayer())
        layer()->updateTransform();
}

IntRect RenderBox::absoluteContentBox() const
{
    IntRect rect = contentBoxRect();
    FloatPoint absPos = localToAbsolute(FloatPoint());
    rect.move(absPos.x(), absPos.y());
    return rect;
}

FloatQuad RenderBox::absoluteContentQuad() const
{
    IntRect rect = contentBoxRect();
    return localToAbsoluteQuad(FloatRect(rect));
}

IntRect RenderBox::outlineBoundsForRepaint(RenderBoxModelObject* repaintContainer, IntPoint* cachedOffsetToRepaintContainer) const
{
    IntRect box = borderBoundingBox();
    adjustRectForOutlineAndShadow(box);

    FloatQuad containerRelativeQuad = FloatRect(box);
    if (cachedOffsetToRepaintContainer)
        containerRelativeQuad.move(cachedOffsetToRepaintContainer->x(), cachedOffsetToRepaintContainer->y());
    else
        containerRelativeQuad = localToContainerQuad(containerRelativeQuad, repaintContainer);

    box = containerRelativeQuad.enclosingBoundingBox();

    // FIXME: layoutDelta needs to be applied in parts before/after transforms and
    // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
    box.move(view()->layoutDelta());

    return box;
}

void RenderBox::addFocusRingRects(Vector<IntRect>& rects, int tx, int ty)
{
    if (width() && height())
        rects.append(IntRect(tx, ty, width(), height()));
}

IntRect RenderBox::reflectionBox() const
{
    IntRect result;
    if (!style()->boxReflect())
        return result;
    IntRect box = borderBoxRect();
    result = box;
    switch (style()->boxReflect()->direction()) {
        case ReflectionBelow:
            result.move(0, box.height() + reflectionOffset());
            break;
        case ReflectionAbove:
            result.move(0, -box.height() - reflectionOffset());
            break;
        case ReflectionLeft:
            result.move(-box.width() - reflectionOffset(), 0);
            break;
        case ReflectionRight:
            result.move(box.width() + reflectionOffset(), 0);
            break;
    }
    return result;
}

int RenderBox::reflectionOffset() const
{
    if (!style()->boxReflect())
        return 0;
    if (style()->boxReflect()->direction() == ReflectionLeft || style()->boxReflect()->direction() == ReflectionRight)
        return style()->boxReflect()->offset().calcValue(borderBoxRect().width());
    return style()->boxReflect()->offset().calcValue(borderBoxRect().height());
}

IntRect RenderBox::reflectedRect(const IntRect& r) const
{
    if (!style()->boxReflect())
        return IntRect();

    IntRect box = borderBoxRect();
    IntRect result = r;
    switch (style()->boxReflect()->direction()) {
        case ReflectionBelow:
            result.setY(box.maxY() + reflectionOffset() + (box.maxY() - r.maxY()));
            break;
        case ReflectionAbove:
            result.setY(box.y() - reflectionOffset() - box.height() + (box.maxY() - r.maxY()));
            break;
        case ReflectionLeft:
            result.setX(box.x() - reflectionOffset() - box.width() + (box.maxX() - r.maxX()));
            break;
        case ReflectionRight:
            result.setX(box.maxX() + reflectionOffset() + (box.maxX() - r.maxX()));
            break;
    }
    return result;
}

bool RenderBox::includeVerticalScrollbarSize() const
{
    return hasOverflowClip() && !layer()->hasOverlayScrollbars()
        && (style()->overflowY() == OSCROLL || style()->overflowY() == OAUTO);
}

bool RenderBox::includeHorizontalScrollbarSize() const
{
    return hasOverflowClip() && !layer()->hasOverlayScrollbars()
        && (style()->overflowX() == OSCROLL || style()->overflowX() == OAUTO);
}

int RenderBox::verticalScrollbarWidth() const
{
    return includeVerticalScrollbarSize() ? layer()->verticalScrollbarWidth() : 0;
}

int RenderBox::horizontalScrollbarHeight() const
{
    return includeHorizontalScrollbarSize() ? layer()->horizontalScrollbarHeight() : 0;
}

bool RenderBox::scroll(ScrollDirection direction, ScrollGranularity granularity, float multiplier, Node** stopNode)
{
    RenderLayer* l = layer();
    if (l && l->scroll(direction, granularity, multiplier)) {
        if (stopNode)
            *stopNode = node();
        return true;
    }

    if (stopNode && *stopNode && *stopNode == node())
        return true;

    RenderBlock* b = containingBlock();
    if (b && !b->isRenderView())
        return b->scroll(direction, granularity, multiplier, stopNode);
    return false;
}

bool RenderBox::logicalScroll(ScrollLogicalDirection direction, ScrollGranularity granularity, float multiplier, Node** stopNode)
{
    bool scrolled = false;
    
    RenderLayer* l = layer();
    if (l) {
#if PLATFORM(MAC)
        // On Mac only we reset the inline direction position when doing a document scroll (e.g., hitting Home/End).
        if (granularity == ScrollByDocument)
            scrolled = l->scroll(logicalToPhysical(ScrollInlineDirectionBackward, isHorizontalWritingMode(), style()->isFlippedBlocksWritingMode()), ScrollByDocument, multiplier);
#endif
        if (l->scroll(logicalToPhysical(direction, isHorizontalWritingMode(), style()->isFlippedBlocksWritingMode()), granularity, multiplier))
            scrolled = true;
        
        if (scrolled) {
            if (stopNode)
                *stopNode = node();
            return true;
        }
    }

    if (stopNode && *stopNode && *stopNode == node())
        return true;

    RenderBlock* b = containingBlock();
    if (b && !b->isRenderView())
        return b->logicalScroll(direction, granularity, multiplier, stopNode);
    return false;
}

bool RenderBox::canBeScrolledAndHasScrollableArea() const
{
    return canBeProgramaticallyScrolled(false) && (scrollHeight() != clientHeight() || scrollWidth() != clientWidth());
}
    
bool RenderBox::canBeProgramaticallyScrolled(bool) const
{
    return (hasOverflowClip() && (scrollsOverflow() || (node() && node()->rendererIsEditable()))) || (node() && node()->isDocumentNode());
}

void RenderBox::autoscroll()
{
    if (layer())
        layer()->autoscroll();
}

void RenderBox::panScroll(const IntPoint& source)
{
    if (layer())
        layer()->panScrollFromPoint(source);
}

int RenderBox::minPreferredLogicalWidth() const
{
    if (preferredLogicalWidthsDirty())
        const_cast<RenderBox*>(this)->computePreferredLogicalWidths();
        
    return m_minPreferredLogicalWidth;
}

int RenderBox::maxPreferredLogicalWidth() const
{
    if (preferredLogicalWidthsDirty())
        const_cast<RenderBox*>(this)->computePreferredLogicalWidths();
        
    return m_maxPreferredLogicalWidth;
}

int RenderBox::overrideSize() const
{
    if (!hasOverrideSize())
        return -1;
    return gOverrideSizeMap->get(this);
}

void RenderBox::setOverrideSize(int s)
{
    if (s == -1) {
        if (hasOverrideSize()) {
            setHasOverrideSize(false);
            gOverrideSizeMap->remove(this);
        }
    } else {
        if (!gOverrideSizeMap)
            gOverrideSizeMap = new OverrideSizeMap();
        setHasOverrideSize(true);
        gOverrideSizeMap->set(this, s);
    }
}

int RenderBox::overrideWidth() const
{
    return hasOverrideSize() ? overrideSize() : width();
}

int RenderBox::overrideHeight() const
{
    return hasOverrideSize() ? overrideSize() : height();
}

int RenderBox::computeBorderBoxLogicalWidth(int width) const
{
    int bordersPlusPadding = borderAndPaddingLogicalWidth();
    if (style()->boxSizing() == CONTENT_BOX)
        return width + bordersPlusPadding;
    return max(width, bordersPlusPadding);
}

int RenderBox::computeBorderBoxLogicalHeight(int height) const
{
    int bordersPlusPadding = borderAndPaddingLogicalHeight();
    if (style()->boxSizing() == CONTENT_BOX)
        return height + bordersPlusPadding;
    return max(height, bordersPlusPadding);
}

int RenderBox::computeContentBoxLogicalWidth(int width) const
{
    if (style()->boxSizing() == BORDER_BOX)
        width -= borderAndPaddingLogicalWidth();
    return max(0, width);
}

int RenderBox::computeContentBoxLogicalHeight(int height) const
{
    if (style()->boxSizing() == BORDER_BOX)
        height -= borderAndPaddingLogicalHeight();
    return max(0, height);
}

// Hit Testing
bool RenderBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int xPos, int yPos, int tx, int ty, HitTestAction action)
{
    tx += x();
    ty += y();

    // Check kids first.
    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        if (!child->hasLayer() && child->nodeAtPoint(request, result, xPos, yPos, tx, ty, action)) {
            updateHitTestResult(result, IntPoint(xPos - tx, yPos - ty));
            return true;
        }
    }

    // Check our bounds next. For this purpose always assume that we can only be hit in the
    // foreground phase (which is true for replaced elements like images).
    IntRect boundsRect = IntRect(tx, ty, width(), height());
    if (visibleToHitTesting() && action == HitTestForeground && boundsRect.intersects(result.rectForPoint(xPos, yPos))) {
        updateHitTestResult(result, IntPoint(xPos - tx, yPos - ty));
        if (!result.addNodeToRectBasedTestResult(node(), xPos, yPos, boundsRect))
            return true;
    }

    return false;
}

// --------------------- painting stuff -------------------------------

void RenderBox::paint(PaintInfo& paintInfo, int tx, int ty)
{
    tx += x();
    ty += y();

    // default implementation. Just pass paint through to the children
    PaintInfo childInfo(paintInfo);
    childInfo.updatePaintingRootForChildren(this);
    for (RenderObject* child = firstChild(); child; child = child->nextSibling())
        child->paint(childInfo, tx, ty);
}

void RenderBox::paintRootBoxFillLayers(const PaintInfo& paintInfo)
{
    const FillLayer* bgLayer = style()->backgroundLayers();
    Color bgColor = style()->visitedDependentColor(CSSPropertyBackgroundColor);
    RenderObject* bodyObject = 0;
    if (!hasBackground() && node() && node()->hasTagName(HTMLNames::htmlTag)) {
        // Locate the <body> element using the DOM.  This is easier than trying
        // to crawl around a render tree with potential :before/:after content and
        // anonymous blocks created by inline <body> tags etc.  We can locate the <body>
        // render object very easily via the DOM.
        HTMLElement* body = document()->body();
        bodyObject = (body && body->hasLocalName(bodyTag)) ? body->renderer() : 0;
        if (bodyObject) {
            bgLayer = bodyObject->style()->backgroundLayers();
            bgColor = bodyObject->style()->visitedDependentColor(CSSPropertyBackgroundColor);
        }
    }

    // The background of the box generated by the root element covers the entire canvas, so just use
    // the RenderView's docTop/Left/Width/Height accessors.
    paintFillLayers(paintInfo, bgColor, bgLayer, view()->docLeft(), view()->docTop(), view()->docWidth(), view()->docHeight(), BackgroundBleedNone, CompositeSourceOver, bodyObject);
}

void RenderBox::paintBoxDecorations(PaintInfo& paintInfo, int tx, int ty)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;
    return paintBoxDecorationsWithSize(paintInfo, tx, ty, width(), height());
}

BackgroundBleedAvoidance RenderBox::determineBackgroundBleedAvoidance(GraphicsContext* context) const
{
    if (context->paintingDisabled())
        return BackgroundBleedNone;

    const RenderStyle* style = this->style();

    if (!style->hasBackground() || !style->hasBorder() || !style->hasBorderRadius() || borderImageIsLoadedAndCanBeRendered())
        return BackgroundBleedNone;

    AffineTransform ctm = context->getCTM();
    FloatSize contextScaling(static_cast<float>(ctm.xScale()), static_cast<float>(ctm.yScale()));
    if (borderObscuresBackgroundEdge(contextScaling))
        return BackgroundBleedShrinkBackground;
    
    // FIXME: there is one more strategy possible, for opaque backgrounds and
    // translucent borders. In that case we could avoid using a transparency layer,
    // and paint the border first, and then paint the background clipped to the
    // inside of the border.

    return BackgroundBleedUseTransparencyLayer;
}

void RenderBox::paintBoxDecorationsWithSize(PaintInfo& paintInfo, int tx, int ty, int width, int height)
{
    // border-fit can adjust where we paint our border and background.  If set, we snugly fit our line box descendants.  (The iChat
    // balloon layout is an example of this).
    borderFitAdjust(tx, width);

    // FIXME: Should eventually give the theme control over whether the box shadow should paint, since controls could have
    // custom shadows of their own.
    paintBoxShadow(paintInfo.context, tx, ty, width, height, style(), Normal);

    BackgroundBleedAvoidance bleedAvoidance = determineBackgroundBleedAvoidance(paintInfo.context);

    GraphicsContextStateSaver stateSaver(*paintInfo.context, false);
    if (bleedAvoidance == BackgroundBleedUseTransparencyLayer) {
        // To avoid the background color bleeding out behind the border, we'll render background and border
        // into a transparency layer, and then clip that in one go (which requires setting up the clip before
        // beginning the layer).
        RoundedIntRect border = style()->getRoundedBorderFor(IntRect(tx, ty, width, height));
        stateSaver.save();
        paintInfo.context->addRoundedRectClip(border);
        paintInfo.context->beginTransparencyLayer(1);
    }
    
    // If we have a native theme appearance, paint that before painting our background.
    // The theme will tell us whether or not we should also paint the CSS background.
    bool themePainted = style()->hasAppearance() && !theme()->paint(this, paintInfo, IntRect(tx, ty, width, height));
    if (!themePainted) {
        if (isRoot())
            paintRootBoxFillLayers(paintInfo);
        else if (!isBody() || document()->documentElement()->renderer()->hasBackground()) {
            // The <body> only paints its background if the root element has defined a background
            // independent of the body.
            paintFillLayers(paintInfo, style()->visitedDependentColor(CSSPropertyBackgroundColor), style()->backgroundLayers(), tx, ty, width, height, bleedAvoidance);
        }
        if (style()->hasAppearance())
            theme()->paintDecorations(this, paintInfo, IntRect(tx, ty, width, height));
    }
    paintBoxShadow(paintInfo.context, tx, ty, width, height, style(), Inset);

    // The theme will tell us whether or not we should also paint the CSS border.
    if ((!style()->hasAppearance() || (!themePainted && theme()->paintBorderOnly(this, paintInfo, IntRect(tx, ty, width, height)))) && style()->hasBorder())
        paintBorder(paintInfo.context, tx, ty, width, height, style(), bleedAvoidance);

    if (bleedAvoidance == BackgroundBleedUseTransparencyLayer)
        paintInfo.context->endTransparencyLayer();
}

void RenderBox::paintMask(PaintInfo& paintInfo, int tx, int ty)
{
    if (!paintInfo.shouldPaintWithinRoot(this) || style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask || paintInfo.context->paintingDisabled())
        return;

    int w = width();
    int h = height();

    // border-fit can adjust where we paint our border and background.  If set, we snugly fit our line box descendants.  (The iChat
    // balloon layout is an example of this).
    borderFitAdjust(tx, w);

    paintMaskImages(paintInfo, tx, ty, w, h);
}

void RenderBox::paintMaskImages(const PaintInfo& paintInfo, int tx, int ty, int w, int h)
{
    // Figure out if we need to push a transparency layer to render our mask.
    bool pushTransparencyLayer = false;
    bool compositedMask = hasLayer() && layer()->hasCompositedMask();
    CompositeOperator compositeOp = CompositeSourceOver;

    bool allMaskImagesLoaded = true;
    
    if (!compositedMask) {
        // If the context has a rotation, scale or skew, then use a transparency layer to avoid
        // pixel cruft around the edge of the mask.
        const AffineTransform& currentCTM = paintInfo.context->getCTM();
        pushTransparencyLayer = !currentCTM.isIdentityOrTranslationOrFlipped();

        StyleImage* maskBoxImage = style()->maskBoxImage().image();
        const FillLayer* maskLayers = style()->maskLayers();

        // Don't render a masked element until all the mask images have loaded, to prevent a flash of unmasked content.
        if (maskBoxImage)
            allMaskImagesLoaded &= maskBoxImage->isLoaded();

        if (maskLayers)
            allMaskImagesLoaded &= maskLayers->imagesAreLoaded();

        // Before all images have loaded, just use an empty transparency layer as the mask.
        if (!allMaskImagesLoaded)
            pushTransparencyLayer = true;

        if (maskBoxImage && maskLayers->hasImage()) {
            // We have a mask-box-image and mask-image, so need to composite them together before using the result as a mask.
            pushTransparencyLayer = true;
        } else {
            // We have to use an extra image buffer to hold the mask. Multiple mask images need
            // to composite together using source-over so that they can then combine into a single unified mask that
            // can be composited with the content using destination-in.  SVG images need to be able to set compositing modes
            // as they draw images contained inside their sub-document, so we paint all our images into a separate buffer
            // and composite that buffer as the mask.
            // We have to check that the mask images to be rendered contain at least one image that can be actually used in rendering
            // before pushing the transparency layer.
            for (const FillLayer* fillLayer = maskLayers->next(); fillLayer; fillLayer = fillLayer->next()) {
                if (fillLayer->hasImage() && fillLayer->image()->canRender(style()->effectiveZoom())) {
                    pushTransparencyLayer = true;
                    // We found one image that can be used in rendering, exit the loop
                    break;
                }
            }
        }
        
        compositeOp = CompositeDestinationIn;
        if (pushTransparencyLayer) {
            paintInfo.context->setCompositeOperation(CompositeDestinationIn);
            paintInfo.context->beginTransparencyLayer(1.0f);
            compositeOp = CompositeSourceOver;
        }
    }

    if (allMaskImagesLoaded) {
        paintFillLayers(paintInfo, Color(), style()->maskLayers(), tx, ty, w, h, BackgroundBleedNone, compositeOp);
        paintNinePieceImage(paintInfo.context, tx, ty, w, h, style(), style()->maskBoxImage(), compositeOp);
    }
    
    if (pushTransparencyLayer)
        paintInfo.context->endTransparencyLayer();
}

IntRect RenderBox::maskClipRect()
{
    IntRect bbox = borderBoxRect();
    if (style()->maskBoxImage().image())
        return bbox;
    
    IntRect result;
    for (const FillLayer* maskLayer = style()->maskLayers(); maskLayer; maskLayer = maskLayer->next()) {
        if (maskLayer->image()) {
            IntRect maskRect;
            IntPoint phase;
            IntSize tileSize;
            calculateBackgroundImageGeometry(maskLayer, bbox.x(), bbox.y(), bbox.width(), bbox.height(), maskRect, phase, tileSize);
            result.unite(maskRect);
        }
    }
    return result;
}

void RenderBox::paintFillLayers(const PaintInfo& paintInfo, const Color& c, const FillLayer* fillLayer, int tx, int ty, int width, int height,
    BackgroundBleedAvoidance bleedAvoidance, CompositeOperator op, RenderObject* backgroundObject)
{
    if (!fillLayer)
        return;

    paintFillLayers(paintInfo, c, fillLayer->next(), tx, ty, width, height, bleedAvoidance, op, backgroundObject);
    paintFillLayer(paintInfo, c, fillLayer, tx, ty, width, height, bleedAvoidance, op, backgroundObject);
}

void RenderBox::paintFillLayer(const PaintInfo& paintInfo, const Color& c, const FillLayer* fillLayer, int tx, int ty, int width, int height,
    BackgroundBleedAvoidance bleedAvoidance, CompositeOperator op, RenderObject* backgroundObject)
{
    paintFillLayerExtended(paintInfo, c, fillLayer, tx, ty, width, height, bleedAvoidance, 0, 0, 0, op, backgroundObject);
}

#if USE(ACCELERATED_COMPOSITING)
static bool layersUseImage(WrappedImagePtr image, const FillLayer* layers)
{
    for (const FillLayer* curLayer = layers; curLayer; curLayer = curLayer->next()) {
        if (curLayer->image() && image == curLayer->image()->data())
            return true;
    }

    return false;
}
#endif

void RenderBox::imageChanged(WrappedImagePtr image, const IntRect*)
{
    if (!parent())
        return;

    if ((style()->borderImage().image() && style()->borderImage().image()->data() == image) ||
        (style()->maskBoxImage().image() && style()->maskBoxImage().image()->data() == image)) {
        repaint();
        return;
    }

    bool didFullRepaint = repaintLayerRectsForImage(image, style()->backgroundLayers(), true);
    if (!didFullRepaint)
        repaintLayerRectsForImage(image, style()->maskLayers(), false);


#if USE(ACCELERATED_COMPOSITING)
    if (hasLayer() && layer()->hasCompositedMask() && layersUseImage(image, style()->maskLayers()))
        layer()->contentChanged(RenderLayer::MaskImageChanged);
#endif
}

bool RenderBox::repaintLayerRectsForImage(WrappedImagePtr image, const FillLayer* layers, bool drawingBackground)
{
    IntRect rendererRect;
    RenderBox* layerRenderer = 0;

    for (const FillLayer* curLayer = layers; curLayer; curLayer = curLayer->next()) {
        if (curLayer->image() && image == curLayer->image()->data() && curLayer->image()->canRender(style()->effectiveZoom())) {
            // Now that we know this image is being used, compute the renderer and the rect
            // if we haven't already
            if (!layerRenderer) {
                bool drawingRootBackground = drawingBackground && (isRoot() || (isBody() && !document()->documentElement()->renderer()->hasBackground()));
                if (drawingRootBackground) {
                    layerRenderer = view();

                    int rw;
                    int rh;

                    if (FrameView* frameView = toRenderView(layerRenderer)->frameView()) {
                        rw = frameView->contentsWidth();
                        rh = frameView->contentsHeight();
                    } else {
                        rw = layerRenderer->width();
                        rh = layerRenderer->height();
                    }
                    rendererRect = IntRect(-layerRenderer->marginLeft(),
                        -layerRenderer->marginTop(),
                        max(layerRenderer->width() + layerRenderer->marginLeft() + layerRenderer->marginRight() + layerRenderer->borderLeft() + layerRenderer->borderRight(), rw),
                        max(layerRenderer->height() + layerRenderer->marginTop() + layerRenderer->marginBottom() + layerRenderer->borderTop() + layerRenderer->borderBottom(), rh));
                } else {
                    layerRenderer = this;
                    rendererRect = borderBoxRect();
                }
            }

            IntRect repaintRect;
            IntPoint phase;
            IntSize tileSize;
            layerRenderer->calculateBackgroundImageGeometry(curLayer, rendererRect.x(), rendererRect.y(), rendererRect.width(), rendererRect.height(), repaintRect, phase, tileSize);
            layerRenderer->repaintRectangle(repaintRect);
            if (repaintRect == rendererRect)
                return true;
        }
    }
    return false;
}

#if PLATFORM(MAC)

void RenderBox::paintCustomHighlight(int tx, int ty, const AtomicString& type, bool behindText)
{
    Frame* frame = this->frame();
    if (!frame)
        return;
    Page* page = frame->page();
    if (!page)
        return;

    InlineBox* boxWrap = inlineBoxWrapper();
    RootInlineBox* r = boxWrap ? boxWrap->root() : 0;
    if (r) {
        FloatRect rootRect(tx + r->x(), ty + r->selectionTop(), r->logicalWidth(), r->selectionHeight());
        FloatRect imageRect(tx + x(), rootRect.y(), width(), rootRect.height());
        page->chrome()->client()->paintCustomHighlight(node(), type, imageRect, rootRect, behindText, false);
    } else {
        FloatRect imageRect(tx + x(), ty + y(), width(), height());
        page->chrome()->client()->paintCustomHighlight(node(), type, imageRect, imageRect, behindText, false);
    }
}

#endif

bool RenderBox::pushContentsClip(PaintInfo& paintInfo, int tx, int ty)
{
    if (paintInfo.phase == PaintPhaseBlockBackground || paintInfo.phase == PaintPhaseSelfOutline || paintInfo.phase == PaintPhaseMask)
        return false;
        
    bool isControlClip = hasControlClip();
    bool isOverflowClip = hasOverflowClip() && !layer()->isSelfPaintingLayer();
    
    if (!isControlClip && !isOverflowClip)
        return false;
    
    if (paintInfo.phase == PaintPhaseOutline)
        paintInfo.phase = PaintPhaseChildOutlines;
    else if (paintInfo.phase == PaintPhaseChildBlockBackground) {
        paintInfo.phase = PaintPhaseBlockBackground;
        paintObject(paintInfo, tx, ty);
        paintInfo.phase = PaintPhaseChildBlockBackgrounds;
    }
    IntRect clipRect(isControlClip ? controlClipRect(tx, ty) : overflowClipRect(tx, ty));
    paintInfo.context->save();
    if (style()->hasBorderRadius())
        paintInfo.context->addRoundedRectClip(style()->getRoundedBorderFor(IntRect(tx, ty, width(), height())));
    paintInfo.context->clip(clipRect);
    return true;
}

void RenderBox::popContentsClip(PaintInfo& paintInfo, PaintPhase originalPhase, int tx, int ty)
{
    ASSERT(hasControlClip() || (hasOverflowClip() && !layer()->isSelfPaintingLayer()));

    paintInfo.context->restore();
    if (originalPhase == PaintPhaseOutline) {
        paintInfo.phase = PaintPhaseSelfOutline;
        paintObject(paintInfo, tx, ty);
        paintInfo.phase = originalPhase;
    } else if (originalPhase == PaintPhaseChildBlockBackground)
        paintInfo.phase = originalPhase;
}

IntRect RenderBox::overflowClipRect(int tx, int ty, OverlayScrollbarSizeRelevancy relevancy)
{
    // FIXME: When overflow-clip (CSS3) is implemented, we'll obtain the property
    // here.

    int bLeft = borderLeft();
    int bTop = borderTop();

    int clipX = tx + bLeft;
    int clipY = ty + bTop;
    int clipWidth = width() - bLeft - borderRight();
    int clipHeight = height() - bTop - borderBottom();

    // Subtract out scrollbars if we have them.
    if (layer()) {
        clipWidth -= layer()->verticalScrollbarWidth(relevancy);
        clipHeight -= layer()->horizontalScrollbarHeight(relevancy);
    }

    return IntRect(clipX, clipY, clipWidth, clipHeight);
}

IntRect RenderBox::clipRect(int tx, int ty)
{
    int clipX = tx;
    int clipY = ty;
    int clipWidth = width();
    int clipHeight = height();

    if (!style()->clipLeft().isAuto()) {
        int c = style()->clipLeft().calcValue(width());
        clipX += c;
        clipWidth -= c;
    }

    if (!style()->clipRight().isAuto())
        clipWidth -= width() - style()->clipRight().calcValue(width());

    if (!style()->clipTop().isAuto()) {
        int c = style()->clipTop().calcValue(height());
        clipY += c;
        clipHeight -= c;
    }

    if (!style()->clipBottom().isAuto())
        clipHeight -= height() - style()->clipBottom().calcValue(height());

    return IntRect(clipX, clipY, clipWidth, clipHeight);
}

int RenderBox::containingBlockLogicalWidthForContent() const
{
    RenderBlock* cb = containingBlock();
    if (shrinkToAvoidFloats())
        return cb->availableLogicalWidthForLine(y(), false);
    return cb->availableLogicalWidth();
}

int RenderBox::perpendicularContainingBlockLogicalHeight() const
{
    RenderBlock* cb = containingBlock();
    RenderStyle* containingBlockStyle = cb->style();
    Length logicalHeightLength = containingBlockStyle->logicalHeight();
    
    // FIXME: For now just support fixed heights.  Eventually should support percentage heights as well.
    if (!logicalHeightLength.isFixed()) {
        // Rather than making the child be completely unconstrained, WinIE uses the viewport width and height
        // as a constraint.  We do that for now as well even though it's likely being unconstrained is what the spec
        // will decide.
        return containingBlockStyle->isHorizontalWritingMode() ? view()->frameView()->visibleHeight() : view()->frameView()->visibleWidth();
    }
    
    // Use the content box logical height as specified by the style.
    return cb->computeContentBoxLogicalHeight(logicalHeightLength.value());
}

void RenderBox::mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool fixed, bool useTransforms, TransformState& transformState) const
{
    if (repaintContainer == this)
        return;

    if (RenderView* v = view()) {
        if (v->layoutStateEnabled() && !repaintContainer) {
            LayoutState* layoutState = v->layoutState();
            IntSize offset = layoutState->m_paintOffset;
            offset.expand(x(), y());
            if (style()->position() == RelativePosition && layer())
                offset += layer()->relativePositionOffset();
            transformState.move(offset);
            return;
        }
    }

    bool containerSkipped;
    RenderObject* o = container(repaintContainer, &containerSkipped);
    if (!o)
        return;

    bool isFixedPos = style()->position() == FixedPosition;
    bool hasTransform = hasLayer() && layer()->transform();
    if (hasTransform) {
        // If this box has a transform, it acts as a fixed position container for fixed descendants,
        // and may itself also be fixed position. So propagate 'fixed' up only if this box is fixed position.
        fixed &= isFixedPos;
    } else
        fixed |= isFixedPos;
    
    IntSize containerOffset = offsetFromContainer(o, roundedIntPoint(transformState.mappedPoint()));
    
    bool preserve3D = useTransforms && (o->style()->preserves3D() || style()->preserves3D());
    if (useTransforms && shouldUseTransformFromContainer(o)) {
        TransformationMatrix t;
        getTransformFromContainer(o, containerOffset, t);
        transformState.applyTransform(t, preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
    } else
        transformState.move(containerOffset.width(), containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);

    if (containerSkipped) {
        // There can't be a transform between repaintContainer and o, because transforms create containers, so it should be safe
        // to just subtract the delta between the repaintContainer and o.
        IntSize containerOffset = repaintContainer->offsetFromAncestorContainer(o);
        transformState.move(-containerOffset.width(), -containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
        return;
    }
    
    o->mapLocalToContainer(repaintContainer, fixed, useTransforms, transformState);
}

void RenderBox::mapAbsoluteToLocalPoint(bool fixed, bool useTransforms, TransformState& transformState) const
{
    // We don't expect absoluteToLocal() to be called during layout (yet)
    ASSERT(!view() || !view()->layoutStateEnabled());
    
    bool isFixedPos = style()->position() == FixedPosition;
    bool hasTransform = hasLayer() && layer()->transform();
    if (hasTransform) {
        // If this box has a transform, it acts as a fixed position container for fixed descendants,
        // and may itself also be fixed position. So propagate 'fixed' up only if this box is fixed position.
        fixed &= isFixedPos;
    } else
        fixed |= isFixedPos;
    
    RenderObject* o = container();
    if (!o)
        return;

    o->mapAbsoluteToLocalPoint(fixed, useTransforms, transformState);

    IntSize containerOffset = offsetFromContainer(o, IntPoint());

    bool preserve3D = useTransforms && (o->style()->preserves3D() || style()->preserves3D());
    if (useTransforms && shouldUseTransformFromContainer(o)) {
        TransformationMatrix t;
        getTransformFromContainer(o, containerOffset, t);
        transformState.applyTransform(t, preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
    } else
        transformState.move(-containerOffset.width(), -containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
}

IntSize RenderBox::offsetFromContainer(RenderObject* o, const IntPoint& point) const
{
    ASSERT(o == container());

    IntSize offset;    
    if (isRelPositioned())
        offset += relativePositionOffset();

    if (!isInline() || isReplaced()) {
        if (style()->position() != AbsolutePosition && style()->position() != FixedPosition) {
            if (o->hasColumns()) {
                IntRect columnRect(frameRect());
                toRenderBlock(o)->flipForWritingModeIncludingColumns(columnRect);
                offset += IntSize(columnRect.location().x(), columnRect.location().y());
                columnRect.move(point.x(), point.y());
                o->adjustForColumns(offset, columnRect.location());
            } else
                offset += locationOffsetIncludingFlipping();
        } else
            offset += locationOffsetIncludingFlipping();
    }

    if (o->hasOverflowClip())
        offset -= toRenderBox(o)->layer()->scrolledContentOffset();

    if (style()->position() == AbsolutePosition && o->isRelPositioned() && o->isRenderInline())
        offset += toRenderInline(o)->relativePositionedInlineOffset(this);

    return offset;
}

InlineBox* RenderBox::createInlineBox()
{
    return new (renderArena()) InlineBox(this);
}

void RenderBox::dirtyLineBoxes(bool fullLayout)
{
    if (m_inlineBoxWrapper) {
        if (fullLayout) {
            m_inlineBoxWrapper->destroy(renderArena());
            m_inlineBoxWrapper = 0;
        } else
            m_inlineBoxWrapper->dirtyLineBoxes();
    }
}

void RenderBox::positionLineBox(InlineBox* box)
{
    if (isPositioned()) {
        // Cache the x position only if we were an INLINE type originally.
        bool wasInline = style()->isOriginalDisplayInlineType();
        if (wasInline) {
            // The value is cached in the xPos of the box.  We only need this value if
            // our object was inline originally, since otherwise it would have ended up underneath
            // the inlines.
            layer()->setStaticInlinePosition(lroundf(box->logicalLeft()));
            if (style()->hasStaticInlinePosition(box->isHorizontal()))
                setChildNeedsLayout(true, false); // Just go ahead and mark the positioned object as needing layout, so it will update its position properly.
        } else {
            // Our object was a block originally, so we make our normal flow position be
            // just below the line box (as though all the inlines that came before us got
            // wrapped in an anonymous block, which is what would have happened had we been
            // in flow).  This value was cached in the y() of the box.
            layer()->setStaticBlockPosition(box->logicalTop());
            if (style()->hasStaticBlockPosition(box->isHorizontal()))
                setChildNeedsLayout(true, false); // Just go ahead and mark the positioned object as needing layout, so it will update its position properly.
        }

        // Nuke the box.
        box->remove();
        box->destroy(renderArena());
    } else if (isReplaced()) {
        setLocation(lroundf(box->x()), lroundf(box->y()));
        if (m_inlineBoxWrapper)
            deleteLineBoxWrapper();
        m_inlineBoxWrapper = box;
    }
}

void RenderBox::deleteLineBoxWrapper()
{
    if (m_inlineBoxWrapper) {
        if (!documentBeingDestroyed())
            m_inlineBoxWrapper->remove();
        m_inlineBoxWrapper->destroy(renderArena());
        m_inlineBoxWrapper = 0;
    }
}

IntRect RenderBox::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer)
{
    if (style()->visibility() != VISIBLE && !enclosingLayer()->hasVisibleContent())
        return IntRect();

    IntRect r = visualOverflowRect();

    RenderView* v = view();
    if (v) {
        // FIXME: layoutDelta needs to be applied in parts before/after transforms and
        // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
        r.move(v->layoutDelta());
    }
    
    if (style()) {
        if (style()->hasAppearance())
            // The theme may wish to inflate the rect used when repainting.
            theme()->adjustRepaintRect(this, r);

        // We have to use maximalOutlineSize() because a child might have an outline
        // that projects outside of our overflowRect.
        if (v) {
            ASSERT(style()->outlineSize() <= v->maximalOutlineSize());
            r.inflate(v->maximalOutlineSize());
        }
    }
    
    computeRectForRepaint(repaintContainer, r);
    return r;
}

void RenderBox::computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& rect, bool fixed)
{
    // The rect we compute at each step is shifted by our x/y offset in the parent container's coordinate space.
    // Only when we cross a writing mode boundary will we have to possibly flipForWritingMode (to convert into a more appropriate
    // offset corner for the enclosing container).  This allows for a fully RL or BT document to repaint
    // properly even during layout, since the rect remains flipped all the way until the end.
    //
    // RenderView::computeRectForRepaint then converts the rect to physical coordinates.  We also convert to
    // physical when we hit a repaintContainer boundary.  Therefore the final rect returned is always in the
    // physical coordinate space of the repaintContainer.
    if (RenderView* v = view()) {
        // LayoutState is only valid for root-relative, non-fixed position repainting
        if (v->layoutStateEnabled() && !repaintContainer && style()->position() != FixedPosition) {
            LayoutState* layoutState = v->layoutState();

            if (layer() && layer()->transform())
                rect = layer()->transform()->mapRect(rect);

            if (style()->position() == RelativePosition && layer())
                rect.move(layer()->relativePositionOffset());

            rect.move(x(), y());
            rect.move(layoutState->m_paintOffset);
            if (layoutState->m_clipped)
                rect.intersect(layoutState->m_clipRect);
            return;
        }
    }

    if (hasReflection())
        rect.unite(reflectedRect(rect));

    if (repaintContainer == this) {
        if (repaintContainer->style()->isFlippedBlocksWritingMode())
            flipForWritingMode(rect);
        return;
    }

    bool containerSkipped;
    RenderObject* o = container(repaintContainer, &containerSkipped);
    if (!o)
        return;

    if (isWritingModeRoot() && !isPositioned())
        flipForWritingMode(rect);
    IntPoint topLeft = rect.location();
    topLeft.move(x(), y());

    EPosition position = style()->position();

    // We are now in our parent container's coordinate space.  Apply our transform to obtain a bounding box
    // in the parent's coordinate space that encloses us.
    if (layer() && layer()->transform()) {
        fixed = position == FixedPosition;
        rect = layer()->transform()->mapRect(rect);
        topLeft = rect.location();
        topLeft.move(x(), y());
    } else if (position == FixedPosition)
        fixed = true;

    if (position == AbsolutePosition && o->isRelPositioned() && o->isRenderInline())
        topLeft += toRenderInline(o)->relativePositionedInlineOffset(this);
    else if (position == RelativePosition && layer()) {
        // Apply the relative position offset when invalidating a rectangle.  The layer
        // is translated, but the render box isn't, so we need to do this to get the
        // right dirty rect.  Since this is called from RenderObject::setStyle, the relative position
        // flag on the RenderObject has been cleared, so use the one on the style().
        topLeft += layer()->relativePositionOffset();
    }
    
    if (o->isBlockFlow() && position != AbsolutePosition && position != FixedPosition) {
        RenderBlock* cb = toRenderBlock(o);
        if (cb->hasColumns()) {
            IntRect repaintRect(topLeft, rect.size());
            cb->adjustRectForColumns(repaintRect);
            topLeft = repaintRect.location();
            rect = repaintRect;
        }
    }

    // FIXME: We ignore the lightweight clipping rect that controls use, since if |o| is in mid-layout,
    // its controlClipRect will be wrong. For overflow clip we use the values cached by the layer.
    if (o->hasOverflowClip()) {
        RenderBox* containerBox = toRenderBox(o);

        // o->height() is inaccurate if we're in the middle of a layout of |o|, so use the
        // layer's size instead.  Even if the layer's size is wrong, the layer itself will repaint
        // anyway if its size does change.
        topLeft -= containerBox->layer()->scrolledContentOffset(); // For overflow:auto/scroll/hidden.

        IntRect repaintRect(topLeft, rect.size());
        IntRect boxRect(0, 0, containerBox->layer()->width(), containerBox->layer()->height());
        rect = intersection(repaintRect, boxRect);
        if (rect.isEmpty())
            return;
    } else
        rect.setLocation(topLeft);

    if (containerSkipped) {
        // If the repaintContainer is below o, then we need to map the rect into repaintContainer's coordinates.
        IntSize containerOffset = repaintContainer->offsetFromAncestorContainer(o);
        rect.move(-containerOffset);
        return;
    }

    o->computeRectForRepaint(repaintContainer, rect, fixed);
}

void RenderBox::repaintDuringLayoutIfMoved(const IntRect& rect)
{
    int newX = x();
    int newY = y();
    int newWidth = width();
    int newHeight = height();
    if (rect.x() != newX || rect.y() != newY) {
        // The child moved.  Invalidate the object's old and new positions.  We have to do this
        // since the object may not have gotten a layout.
        m_frameRect = rect;
        repaint();
        repaintOverhangingFloats(true);
        m_frameRect = IntRect(newX, newY, newWidth, newHeight);
        repaint();
        repaintOverhangingFloats(true);
    }
}

void RenderBox::computeLogicalWidth()
{
    if (isPositioned()) {
        // FIXME: This calculation is not patched for block-flow yet.
        // https://bugs.webkit.org/show_bug.cgi?id=46500
        computePositionedLogicalWidth();
        return;
    }

    // If layout is limited to a subtree, the subtree root's logical width does not change.
    if (node() && view()->frameView() && view()->frameView()->layoutRoot(true) == this)
        return;

    // The parent box is flexing us, so it has increased or decreased our
    // width.  Use the width from the style context.
    // FIXME: Account for block-flow in flexible boxes.
    // https://bugs.webkit.org/show_bug.cgi?id=46418
    if (hasOverrideSize() &&  parent()->style()->boxOrient() == HORIZONTAL
            && parent()->isFlexibleBox() && parent()->isFlexingChildren()) {
        setLogicalWidth(overrideSize());
        return;
    }

    // FIXME: Account for block-flow in flexible boxes.
    // https://bugs.webkit.org/show_bug.cgi?id=46418
    bool inVerticalBox = parent()->isFlexibleBox() && (parent()->style()->boxOrient() == VERTICAL);
    bool stretching = (parent()->style()->boxAlign() == BSTRETCH);
    bool treatAsReplaced = shouldComputeSizeAsReplaced() && (!inVerticalBox || !stretching);

    Length logicalWidthLength = (treatAsReplaced) ? Length(computeReplacedLogicalWidth(), Fixed) : style()->logicalWidth();

    RenderBlock* cb = containingBlock();
    int containerLogicalWidth = max(0, containingBlockLogicalWidthForContent());
    bool hasPerpendicularContainingBlock = cb->isHorizontalWritingMode() != isHorizontalWritingMode();
    int containerWidthInInlineDirection = containerLogicalWidth;
    if (hasPerpendicularContainingBlock)
        containerWidthInInlineDirection = perpendicularContainingBlockLogicalHeight();
    
    if (isInline() && !isInlineBlockOrInlineTable()) {
        // just calculate margins
        setMarginStart(style()->marginStart().calcMinValue(containerLogicalWidth));
        setMarginEnd(style()->marginEnd().calcMinValue(containerLogicalWidth));
        if (treatAsReplaced)
            setLogicalWidth(max(logicalWidthLength.value() + borderAndPaddingLogicalWidth(), minPreferredLogicalWidth()));
        return;
    }

    // Width calculations
    if (treatAsReplaced)
        setLogicalWidth(logicalWidthLength.value() + borderAndPaddingLogicalWidth());
    else {
        // Calculate LogicalWidth
        setLogicalWidth(computeLogicalWidthUsing(LogicalWidth, containerWidthInInlineDirection));

        // Calculate MaxLogicalWidth
        if (!style()->logicalMaxWidth().isUndefined()) {
            int maxLogicalWidth = computeLogicalWidthUsing(MaxLogicalWidth, containerWidthInInlineDirection);
            if (logicalWidth() > maxLogicalWidth) {
                setLogicalWidth(maxLogicalWidth);
                logicalWidthLength = style()->logicalMaxWidth();
            }
        }

        // Calculate MinLogicalWidth
        int minLogicalWidth = computeLogicalWidthUsing(MinLogicalWidth, containerWidthInInlineDirection);
        if (logicalWidth() < minLogicalWidth) {
            setLogicalWidth(minLogicalWidth);
            logicalWidthLength = style()->logicalMinWidth();
        }
    }

    // Fieldsets are currently the only objects that stretch to their minimum width.
    if (stretchesToMinIntrinsicLogicalWidth()) {
        setLogicalWidth(max(logicalWidth(), minPreferredLogicalWidth()));
        logicalWidthLength = Length(logicalWidth(), Fixed);
    }

    // Margin calculations.
    if (logicalWidthLength.isAuto() || hasPerpendicularContainingBlock || isFloating() || isInline()) {
        setMarginStart(style()->marginStart().calcMinValue(containerLogicalWidth));
        setMarginEnd(style()->marginEnd().calcMinValue(containerLogicalWidth));
    } else
        computeInlineDirectionMargins(cb, containerLogicalWidth, logicalWidth());

    if (!hasPerpendicularContainingBlock && containerLogicalWidth && containerLogicalWidth != (logicalWidth() + marginStart() + marginEnd())
            && !isFloating() && !isInline() && !cb->isFlexibleBox())
        cb->setMarginEndForChild(this, containerLogicalWidth - logicalWidth() - cb->marginStartForChild(this));
}

int RenderBox::computeLogicalWidthUsing(LogicalWidthType widthType, int availableLogicalWidth)
{
    int logicalWidthResult = logicalWidth();
    Length logicalWidth;
    if (widthType == LogicalWidth)
        logicalWidth = style()->logicalWidth();
    else if (widthType == MinLogicalWidth)
        logicalWidth = style()->logicalMinWidth();
    else
        logicalWidth = style()->logicalMaxWidth();

    if (logicalWidth.isIntrinsicOrAuto()) {
        int marginStart = style()->marginStart().calcMinValue(availableLogicalWidth);
        int marginEnd = style()->marginEnd().calcMinValue(availableLogicalWidth);
        if (availableLogicalWidth)
            logicalWidthResult = availableLogicalWidth - marginStart - marginEnd;

        if (sizesToIntrinsicLogicalWidth(widthType)) {
            logicalWidthResult = max(logicalWidthResult, minPreferredLogicalWidth());
            logicalWidthResult = min(logicalWidthResult, maxPreferredLogicalWidth());
        }
    } else // FIXME: If the containing block flow is perpendicular to our direction we need to use the available logical height instead.
        logicalWidthResult = computeBorderBoxLogicalWidth(logicalWidth.calcValue(availableLogicalWidth)); 

    return logicalWidthResult;
}

bool RenderBox::sizesToIntrinsicLogicalWidth(LogicalWidthType widthType) const
{
    // Marquees in WinIE are like a mixture of blocks and inline-blocks.  They size as though they're blocks,
    // but they allow text to sit on the same line as the marquee.
    if (isFloating() || (isInlineBlockOrInlineTable() && !isHTMLMarquee()))
        return true;

    // This code may look a bit strange.  Basically width:intrinsic should clamp the size when testing both
    // min-width and width.  max-width is only clamped if it is also intrinsic.
    Length logicalWidth = (widthType == MaxLogicalWidth) ? style()->logicalMaxWidth() : style()->logicalWidth();
    if (logicalWidth.type() == Intrinsic)
        return true;

    // Children of a horizontal marquee do not fill the container by default.
    // FIXME: Need to deal with MAUTO value properly.  It could be vertical.
    // FIXME: Think about block-flow here.  Need to find out how marquee direction relates to
    // block-flow (as well as how marquee overflow should relate to block flow).
    // https://bugs.webkit.org/show_bug.cgi?id=46472
    if (parent()->style()->overflowX() == OMARQUEE) {
        EMarqueeDirection dir = parent()->style()->marqueeDirection();
        if (dir == MAUTO || dir == MFORWARD || dir == MBACKWARD || dir == MLEFT || dir == MRIGHT)
            return true;
    }

    // Flexible horizontal boxes lay out children at their intrinsic widths.  Also vertical boxes
    // that don't stretch their kids lay out their children at their intrinsic widths.
    // FIXME: Think about block-flow here.
    // https://bugs.webkit.org/show_bug.cgi?id=46473
    if (parent()->isFlexibleBox()
            && (parent()->style()->boxOrient() == HORIZONTAL || parent()->style()->boxAlign() != BSTRETCH))
        return true;

    // Button, input, select, textarea, and legend treat
    // width value of 'auto' as 'intrinsic' unless it's in a
    // stretching vertical flexbox.
    // FIXME: Think about block-flow here.
    // https://bugs.webkit.org/show_bug.cgi?id=46473
    if (logicalWidth.type() == Auto && !(parent()->isFlexibleBox() && parent()->style()->boxOrient() == VERTICAL && parent()->style()->boxAlign() == BSTRETCH) && node() && (node()->hasTagName(inputTag) || node()->hasTagName(selectTag) || node()->hasTagName(buttonTag) || node()->hasTagName(textareaTag) || node()->hasTagName(legendTag)))
        return true;

    return false;
}

void RenderBox::computeInlineDirectionMargins(RenderBlock* containingBlock, int containerWidth, int childWidth)
{
    const RenderStyle* containingBlockStyle = containingBlock->style();
    Length marginStartLength = style()->marginStartUsing(containingBlockStyle);
    Length marginEndLength = style()->marginEndUsing(containingBlockStyle);
    
    // Case One: The object is being centered in the containing block's available logical width.
    if ((marginStartLength.isAuto() && marginEndLength.isAuto() && childWidth < containerWidth)
        || (!marginStartLength.isAuto() && !marginEndLength.isAuto() && containingBlock->style()->textAlign() == WEBKIT_CENTER)) {
        containingBlock->setMarginStartForChild(this, max(0, (containerWidth - childWidth) / 2));
        containingBlock->setMarginEndForChild(this, containerWidth - childWidth - containingBlock->marginStartForChild(this));
        return;
    } 
    
    // Case Two: The object is being pushed to the start of the containing block's available logical width.
    if (marginEndLength.isAuto() && childWidth < containerWidth) {
        containingBlock->setMarginStartForChild(this, marginStartLength.calcValue(containerWidth));
        containingBlock->setMarginEndForChild(this, containerWidth - childWidth - containingBlock->marginStartForChild(this));
        return;
    } 
    
    // Case Three: The object is being pushed to the end of the containing block's available logical width.
    bool pushToEndFromTextAlign = !marginEndLength.isAuto() && ((!containingBlockStyle->isLeftToRightDirection() && containingBlockStyle->textAlign() == WEBKIT_LEFT)
        || (containingBlockStyle->isLeftToRightDirection() && containingBlockStyle->textAlign() == WEBKIT_RIGHT));
    if ((marginStartLength.isAuto() && childWidth < containerWidth) || pushToEndFromTextAlign) {
        containingBlock->setMarginEndForChild(this, marginEndLength.calcValue(containerWidth));
        containingBlock->setMarginStartForChild(this, containerWidth - childWidth - containingBlock->marginEndForChild(this));
        return;
    } 
    
    // Case Four: Either no auto margins, or our width is >= the container width (css2.1, 10.3.3).  In that case
    // auto margins will just turn into 0.
    containingBlock->setMarginStartForChild(this, marginStartLength.calcMinValue(containerWidth));
    containingBlock->setMarginEndForChild(this, marginEndLength.calcMinValue(containerWidth));
}

void RenderBox::computeLogicalHeight()
{
    // Cell height is managed by the table and inline non-replaced elements do not support a height property.
    if (isTableCell() || (isInline() && !isReplaced()))
        return;

    Length h;
    if (isPositioned()) {
        // FIXME: This calculation is not patched for block-flow yet.
        // https://bugs.webkit.org/show_bug.cgi?id=46500
        computePositionedLogicalHeight();
    } else {
        RenderBlock* cb = containingBlock();
        bool hasPerpendicularContainingBlock = cb->isHorizontalWritingMode() != isHorizontalWritingMode();
    
        if (!hasPerpendicularContainingBlock)
            computeBlockDirectionMargins(cb);

        // For tables, calculate margins only.
        if (isTable()) {
            if (hasPerpendicularContainingBlock)
                computeInlineDirectionMargins(cb, containingBlockLogicalWidthForContent(), logicalHeight());
            return;
        }

        // FIXME: Account for block-flow in flexible boxes.
        // https://bugs.webkit.org/show_bug.cgi?id=46418
        bool inHorizontalBox = parent()->isFlexibleBox() && parent()->style()->boxOrient() == HORIZONTAL;
        bool stretching = parent()->style()->boxAlign() == BSTRETCH;
        bool treatAsReplaced = shouldComputeSizeAsReplaced() && (!inHorizontalBox || !stretching);
        bool checkMinMaxHeight = false;

        // The parent box is flexing us, so it has increased or decreased our height.  We have to
        // grab our cached flexible height.
        // FIXME: Account for block-flow in flexible boxes.
        // https://bugs.webkit.org/show_bug.cgi?id=46418
        if (hasOverrideSize() && parent()->isFlexibleBox() && parent()->style()->boxOrient() == VERTICAL
                && parent()->isFlexingChildren())
            h = Length(overrideSize() - borderAndPaddingLogicalHeight(), Fixed);
        else if (treatAsReplaced)
            h = Length(computeReplacedLogicalHeight(), Fixed);
        else {
            h = style()->logicalHeight();
            checkMinMaxHeight = true;
        }

        // Block children of horizontal flexible boxes fill the height of the box.
        // FIXME: Account for block-flow in flexible boxes.
        // https://bugs.webkit.org/show_bug.cgi?id=46418
        if (h.isAuto() && parent()->isFlexibleBox() && parent()->style()->boxOrient() == HORIZONTAL
                && parent()->isStretchingChildren()) {
            h = Length(parentBox()->contentLogicalHeight() - marginBefore() - marginAfter() - borderAndPaddingLogicalHeight(), Fixed);
            checkMinMaxHeight = false;
        }

        int heightResult;
        if (checkMinMaxHeight) {
            heightResult = computeLogicalHeightUsing(style()->logicalHeight());
            if (heightResult == -1)
                heightResult = logicalHeight();
            int minH = computeLogicalHeightUsing(style()->logicalMinHeight()); // Leave as -1 if unset.
            int maxH = style()->logicalMaxHeight().isUndefined() ? heightResult : computeLogicalHeightUsing(style()->logicalMaxHeight());
            if (maxH == -1)
                maxH = heightResult;
            heightResult = min(maxH, heightResult);
            heightResult = max(minH, heightResult);
        } else {
            // The only times we don't check min/max height are when a fixed length has
            // been given as an override.  Just use that.  The value has already been adjusted
            // for box-sizing.
            heightResult = h.value() + borderAndPaddingLogicalHeight();
        }

        setLogicalHeight(heightResult);
        
        if (hasPerpendicularContainingBlock)
            computeInlineDirectionMargins(cb, containingBlockLogicalWidthForContent(), heightResult);
    }

    // WinIE quirk: The <html> block always fills the entire canvas in quirks mode.  The <body> always fills the
    // <html> block in quirks mode.  Only apply this quirk if the block is normal flow and no height
    // is specified. When we're printing, we also need this quirk if the body or root has a percentage 
    // height since we don't set a height in RenderView when we're printing. So without this quirk, the 
    // height has nothing to be a percentage of, and it ends up being 0. That is bad.
    bool paginatedContentNeedsBaseHeight = document()->printing() && h.isPercent()
        && (isRoot() || (isBody() && document()->documentElement()->renderer()->style()->logicalHeight().isPercent()));
    if (stretchesToViewport() || paginatedContentNeedsBaseHeight) {
        // FIXME: Finish accounting for block flow here.
        // https://bugs.webkit.org/show_bug.cgi?id=46603
        int margins = collapsedMarginBefore() + collapsedMarginAfter();
        int visHeight;
        if (document()->printing())
            visHeight = static_cast<int>(view()->pageLogicalHeight());
        else  {
            if (isHorizontalWritingMode())
                visHeight = view()->viewHeight();
            else
                visHeight = view()->viewWidth();
        }
        if (isRoot())
            setLogicalHeight(max(logicalHeight(), visHeight - margins));
        else {
            int marginsBordersPadding = margins + parentBox()->marginBefore() + parentBox()->marginAfter() + parentBox()->borderAndPaddingLogicalHeight();
            setLogicalHeight(max(logicalHeight(), visHeight - marginsBordersPadding));
        }
    }
}

int RenderBox::computeLogicalHeightUsing(const Length& h)
{
    int logicalHeight = -1;
    if (!h.isAuto()) {
        if (h.isFixed())
            logicalHeight = h.value();
        else if (h.isPercent())
            logicalHeight = computePercentageLogicalHeight(h);
        if (logicalHeight != -1) {
            logicalHeight = computeBorderBoxLogicalHeight(logicalHeight);
            return logicalHeight;
        }
    }
    return logicalHeight;
}

int RenderBox::computePercentageLogicalHeight(const Length& height)
{
    int result = -1;
    
    // In quirks mode, blocks with auto height are skipped, and we keep looking for an enclosing
    // block that may have a specified height and then use it. In strict mode, this violates the
    // specification, which states that percentage heights just revert to auto if the containing
    // block has an auto height. We still skip anonymous containing blocks in both modes, though, and look
    // only at explicit containers.
    bool skippedAutoHeightContainingBlock = false;
    RenderBlock* cb = containingBlock();
    while (!cb->isRenderView() && !cb->isBody() && !cb->isTableCell() && !cb->isPositioned() && cb->style()->logicalHeight().isAuto()) {
        if (!document()->inQuirksMode() && !cb->isAnonymousBlock())
            break;
        skippedAutoHeightContainingBlock = true;
        cb = cb->containingBlock();
        cb->addPercentHeightDescendant(this);
    }

    // A positioned element that specified both top/bottom or that specifies height should be treated as though it has a height
    // explicitly specified that can be used for any percentage computations.
    // FIXME: We can't just check top/bottom here.
    // https://bugs.webkit.org/show_bug.cgi?id=46500
    bool isPositionedWithSpecifiedHeight = cb->isPositioned() && (!cb->style()->logicalHeight().isAuto() || (!cb->style()->top().isAuto() && !cb->style()->bottom().isAuto()));

    bool includeBorderPadding = isTable();

    // Table cells violate what the CSS spec says to do with heights.  Basically we
    // don't care if the cell specified a height or not.  We just always make ourselves
    // be a percentage of the cell's current content height.
    if (cb->isTableCell()) {
        if (!skippedAutoHeightContainingBlock) {
            result = cb->overrideSize();
            if (result == -1) {
                // Normally we would let the cell size intrinsically, but scrolling overflow has to be
                // treated differently, since WinIE lets scrolled overflow regions shrink as needed.
                // While we can't get all cases right, we can at least detect when the cell has a specified
                // height or when the table has a specified height.  In these cases we want to initially have
                // no size and allow the flexing of the table or the cell to its specified height to cause us
                // to grow to fill the space.  This could end up being wrong in some cases, but it is
                // preferable to the alternative (sizing intrinsically and making the row end up too big).
                RenderTableCell* cell = toRenderTableCell(cb);
                if (scrollsOverflowY() && (!cell->style()->logicalHeight().isAuto() || !cell->table()->style()->logicalHeight().isAuto()))
                    return 0;
                return -1;
            }
            includeBorderPadding = true;
        }
    }
    // Otherwise we only use our percentage height if our containing block had a specified
    // height.
    else if (cb->style()->logicalHeight().isFixed())
        result = cb->computeContentBoxLogicalHeight(cb->style()->logicalHeight().value());
    else if (cb->style()->logicalHeight().isPercent() && !isPositionedWithSpecifiedHeight) {
        // We need to recur and compute the percentage height for our containing block.
        result = cb->computePercentageLogicalHeight(cb->style()->logicalHeight());
        if (result != -1)
            result = cb->computeContentBoxLogicalHeight(result);
    } else if (cb->isRenderView() || (cb->isBody() && document()->inQuirksMode()) || isPositionedWithSpecifiedHeight) {
        // Don't allow this to affect the block' height() member variable, since this
        // can get called while the block is still laying out its kids.
        int oldHeight = cb->logicalHeight();
        cb->computeLogicalHeight();
        result = cb->contentLogicalHeight();
        cb->setLogicalHeight(oldHeight);
    } else if (cb->isRoot() && isPositioned())
        // Match the positioned objects behavior, which is that positioned objects will fill their viewport
        // always.  Note we could only hit this case by recurring into computePercentageLogicalHeight on a positioned containing block.
        result = cb->computeContentBoxLogicalHeight(cb->availableLogicalHeight());

    if (result != -1) {
        result = height.calcValue(result);
        if (includeBorderPadding) {
            // It is necessary to use the border-box to match WinIE's broken
            // box model.  This is essential for sizing inside
            // table cells using percentage heights.
            result -= borderAndPaddingLogicalHeight();
            result = max(0, result);
        }
    }
    return result;
}

int RenderBox::computeReplacedLogicalWidth(bool includeMaxWidth) const
{
    int logicalWidth = computeReplacedLogicalWidthUsing(style()->logicalWidth());
    int minLogicalWidth = computeReplacedLogicalWidthUsing(style()->logicalMinWidth());
    int maxLogicalWidth = !includeMaxWidth || style()->logicalMaxWidth().isUndefined() ? logicalWidth : computeReplacedLogicalWidthUsing(style()->logicalMaxWidth());

    return max(minLogicalWidth, min(logicalWidth, maxLogicalWidth));
}

int RenderBox::computeReplacedLogicalWidthUsing(Length logicalWidth) const
{
    switch (logicalWidth.type()) {
        case Fixed:
            return computeContentBoxLogicalWidth(logicalWidth.value());
        case Percent: {
            // FIXME: containingBlockLogicalWidthForContent() is wrong if the replaced element's block-flow is perpendicular to the
            // containing block's block-flow.
            // https://bugs.webkit.org/show_bug.cgi?id=46496
            const int cw = isPositioned() ? containingBlockLogicalWidthForPositioned(toRenderBoxModelObject(container())) : containingBlockLogicalWidthForContent();
            if (cw > 0)
                return computeContentBoxLogicalWidth(logicalWidth.calcMinValue(cw));
        }
        // fall through
        default:
            return intrinsicLogicalWidth();
     }
}

int RenderBox::computeReplacedLogicalHeight() const
{
    int logicalHeight = computeReplacedLogicalHeightUsing(style()->logicalHeight());
    int minLogicalHeight = computeReplacedLogicalHeightUsing(style()->logicalMinHeight());
    int maxLogicalHeight = style()->logicalMaxHeight().isUndefined() ? logicalHeight : computeReplacedLogicalHeightUsing(style()->logicalMaxHeight());

    return max(minLogicalHeight, min(logicalHeight, maxLogicalHeight));
}

int RenderBox::computeReplacedLogicalHeightUsing(Length logicalHeight) const
{
    switch (logicalHeight.type()) {
        case Fixed:
            return computeContentBoxLogicalHeight(logicalHeight.value());
        case Percent:
        {
            RenderObject* cb = isPositioned() ? container() : containingBlock();
            while (cb->isAnonymous()) {
                cb = cb->containingBlock();
                toRenderBlock(cb)->addPercentHeightDescendant(const_cast<RenderBox*>(this));
            }

            // FIXME: This calculation is not patched for block-flow yet.
            // https://bugs.webkit.org/show_bug.cgi?id=46500
            if (cb->isPositioned() && cb->style()->height().isAuto() && !(cb->style()->top().isAuto() || cb->style()->bottom().isAuto())) {
                ASSERT(cb->isRenderBlock());
                RenderBlock* block = toRenderBlock(cb);
                int oldHeight = block->height();
                block->computeLogicalHeight();
                int newHeight = block->computeContentBoxLogicalHeight(block->contentHeight());
                block->setHeight(oldHeight);
                return computeContentBoxLogicalHeight(logicalHeight.calcValue(newHeight));
            }
            
            // FIXME: availableLogicalHeight() is wrong if the replaced element's block-flow is perpendicular to the
            // containing block's block-flow.
            // https://bugs.webkit.org/show_bug.cgi?id=46496
            int availableHeight = isPositioned() ? containingBlockLogicalHeightForPositioned(toRenderBoxModelObject(cb)) : toRenderBox(cb)->availableLogicalHeight();

            // It is necessary to use the border-box to match WinIE's broken
            // box model.  This is essential for sizing inside
            // table cells using percentage heights.
            // FIXME: This needs to be made block-flow-aware.  If the cell and image are perpendicular block-flows, this isn't right.
            // https://bugs.webkit.org/show_bug.cgi?id=46997
            while (cb && !cb->isRenderView() && (cb->style()->logicalHeight().isAuto() || cb->style()->logicalHeight().isPercent())) {
                if (cb->isTableCell()) {
                    // Don't let table cells squeeze percent-height replaced elements
                    // <http://bugs.webkit.org/show_bug.cgi?id=15359>
                    availableHeight = max(availableHeight, intrinsicLogicalHeight());
                    return logicalHeight.calcValue(availableHeight - borderAndPaddingLogicalHeight());
                }
                cb = cb->containingBlock();
            }

            return computeContentBoxLogicalHeight(logicalHeight.calcValue(availableHeight));
        }
        default:
            return intrinsicLogicalHeight();
    }
}

int RenderBox::availableLogicalHeight() const
{
    return availableLogicalHeightUsing(style()->logicalHeight());
}

int RenderBox::availableLogicalHeightUsing(const Length& h) const
{
    if (h.isFixed())
        return computeContentBoxLogicalHeight(h.value());

    if (isRenderView())
        return isHorizontalWritingMode() ? toRenderView(this)->frameView()->visibleHeight() : toRenderView(this)->frameView()->visibleWidth();

    // We need to stop here, since we don't want to increase the height of the table
    // artificially.  We're going to rely on this cell getting expanded to some new
    // height, and then when we lay out again we'll use the calculation below.
    if (isTableCell() && (h.isAuto() || h.isPercent()))
        return overrideSize() - borderAndPaddingLogicalWidth();

    if (h.isPercent())
       return computeContentBoxLogicalHeight(h.calcValue(containingBlock()->availableLogicalHeight()));

    // FIXME: We can't just check top/bottom here.
    // https://bugs.webkit.org/show_bug.cgi?id=46500
    if (isRenderBlock() && isPositioned() && style()->height().isAuto() && !(style()->top().isAuto() || style()->bottom().isAuto())) {
        RenderBlock* block = const_cast<RenderBlock*>(toRenderBlock(this));
        int oldHeight = block->logicalHeight();
        block->computeLogicalHeight();
        int newHeight = block->computeContentBoxLogicalHeight(block->contentLogicalHeight());
        block->setLogicalHeight(oldHeight);
        return computeContentBoxLogicalHeight(newHeight);
    }

    return containingBlock()->availableLogicalHeight();
}

void RenderBox::computeBlockDirectionMargins(RenderBlock* containingBlock)
{
    if (isTableCell()) {
        // FIXME: Not right if we allow cells to have different directionality than the table.  If we do allow this, though,
        // we may just do it with an extra anonymous block inside the cell.
        setMarginBefore(0);
        setMarginAfter(0);
        return;
    }

    // Margins are calculated with respect to the logical width of
    // the containing block (8.3)
    int cw = containingBlockLogicalWidthForContent();

    RenderStyle* containingBlockStyle = containingBlock->style();
    containingBlock->setMarginBeforeForChild(this, style()->marginBeforeUsing(containingBlockStyle).calcMinValue(cw));
    containingBlock->setMarginAfterForChild(this, style()->marginAfterUsing(containingBlockStyle).calcMinValue(cw));
}

int RenderBox::containingBlockLogicalWidthForPositioned(const RenderBoxModelObject* containingBlock, bool checkForPerpendicularWritingMode) const
{
    if (checkForPerpendicularWritingMode && containingBlock->isHorizontalWritingMode() != isHorizontalWritingMode())
        return containingBlockLogicalHeightForPositioned(containingBlock, false);

    if (containingBlock->isBox())
        return toRenderBox(containingBlock)->clientLogicalWidth();

    ASSERT(containingBlock->isRenderInline() && containingBlock->isRelPositioned());

    const RenderInline* flow = toRenderInline(containingBlock);
    InlineFlowBox* first = flow->firstLineBox();
    InlineFlowBox* last = flow->lastLineBox();

    // If the containing block is empty, return a width of 0.
    if (!first || !last)
        return 0;

    int fromLeft;
    int fromRight;
    if (containingBlock->style()->isLeftToRightDirection()) {
        fromLeft = first->logicalLeft() + first->borderLogicalLeft();
        fromRight = last->logicalLeft() + last->logicalWidth() - last->borderLogicalRight();
    } else {
        fromRight = first->logicalLeft() + first->logicalWidth() - first->borderLogicalRight();
        fromLeft = last->logicalLeft() + last->borderLogicalLeft();
    }

    return max(0, (fromRight - fromLeft));
}

int RenderBox::containingBlockLogicalHeightForPositioned(const RenderBoxModelObject* containingBlock, bool checkForPerpendicularWritingMode) const
{
    if (checkForPerpendicularWritingMode && containingBlock->isHorizontalWritingMode() != isHorizontalWritingMode())
        return containingBlockLogicalWidthForPositioned(containingBlock, false);

    if (containingBlock->isBox())
        return toRenderBox(containingBlock)->clientLogicalHeight();
        
    ASSERT(containingBlock->isRenderInline() && containingBlock->isRelPositioned());

    const RenderInline* flow = toRenderInline(containingBlock);
    InlineFlowBox* first = flow->firstLineBox();
    InlineFlowBox* last = flow->lastLineBox();

    // If the containing block is empty, return a height of 0.
    if (!first || !last)
        return 0;

    int heightResult;
    IntRect boundingBox = flow->linesBoundingBox();
    if (containingBlock->isHorizontalWritingMode())
        heightResult = boundingBox.height();
    else
        heightResult = boundingBox.width();
    heightResult -= (containingBlock->borderBefore() + containingBlock->borderAfter());
    return heightResult;
}

static void computeInlineStaticDistance(Length& logicalLeft, Length& logicalRight, const RenderBox* child, const RenderBoxModelObject* containerBlock, int containerLogicalWidth,
                                        TextDirection containerDirection)
{
    if (!logicalLeft.isAuto() || !logicalRight.isAuto())
        return;

    // FIXME: The static distance computation has not been patched for mixed writing modes yet.
    if (containerDirection == LTR) {
        int staticPosition = child->layer()->staticInlinePosition() - containerBlock->borderLogicalLeft();
        for (RenderObject* curr = child->parent(); curr && curr != containerBlock; curr = curr->container()) {
            if (curr->isBox())
                staticPosition += toRenderBox(curr)->logicalLeft();
        }
        logicalLeft.setValue(Fixed, staticPosition);
    } else {
        RenderBox* enclosingBox = child->parent()->enclosingBox();
        int staticPosition = child->layer()->staticInlinePosition() + containerLogicalWidth + containerBlock->borderLogicalRight();
        staticPosition -= enclosingBox->logicalWidth();
        for (RenderObject* curr = enclosingBox; curr && curr != containerBlock; curr = curr->container()) {
            if (curr->isBox())
                staticPosition -= toRenderBox(curr)->logicalLeft();
        }
        logicalRight.setValue(Fixed, staticPosition);
    }
}

void RenderBox::computePositionedLogicalWidth()
{
    if (isReplaced()) {
        computePositionedLogicalWidthReplaced();
        return;
    }

    // QUESTIONS
    // FIXME 1: Which RenderObject's 'direction' property should used: the
    // containing block (cb) as the spec seems to imply, the parent (parent()) as
    // was previously done in calculating the static distances, or ourself, which
    // was also previously done for deciding what to override when you had
    // over-constrained margins?  Also note that the container block is used
    // in similar situations in other parts of the RenderBox class (see computeLogicalWidth()
    // and computeMarginsInContainingBlockInlineDirection()). For now we are using the parent for quirks
    // mode and the containing block for strict mode.

    // FIXME 2: Should we still deal with these the cases of 'left' or 'right' having
    // the type 'static' in determining whether to calculate the static distance?
    // NOTE: 'static' is not a legal value for 'left' or 'right' as of CSS 2.1.

    // FIXME 3: Can perhaps optimize out cases when max-width/min-width are greater
    // than or less than the computed width().  Be careful of box-sizing and
    // percentage issues.

    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.3.7 "Absolutely positioned, non-replaced elements"
    // <http://www.w3.org/TR/CSS21/visudet.html#abs-non-replaced-width>
    // (block-style-comments in this function and in computePositionedLogicalWidthUsing()
    // correspond to text from the spec)


    // We don't use containingBlock(), since we may be positioned by an enclosing
    // relative positioned inline.
    const RenderBoxModelObject* containerBlock = toRenderBoxModelObject(container());
    
    const int containerLogicalWidth = containingBlockLogicalWidthForPositioned(containerBlock);

    // To match WinIE, in quirks mode use the parent's 'direction' property
    // instead of the the container block's.
    TextDirection containerDirection = (document()->inQuirksMode()) ? parent()->style()->direction() : containerBlock->style()->direction();

    bool isHorizontal = isHorizontalWritingMode();
    const int bordersPlusPadding = borderAndPaddingLogicalWidth();
    const Length marginLogicalLeft = isHorizontal ? style()->marginLeft() : style()->marginTop();
    const Length marginLogicalRight = isHorizontal ? style()->marginRight() : style()->marginBottom();
    int& marginLogicalLeftAlias = isHorizontal ? m_marginLeft : m_marginTop;
    int& marginLogicalRightAlias = isHorizontal ? m_marginRight : m_marginBottom;

    Length logicalLeft = style()->logicalLeft();
    Length logicalRight = style()->logicalRight();

    /*---------------------------------------------------------------------------*\
     * For the purposes of this section and the next, the term "static position"
     * (of an element) refers, roughly, to the position an element would have had
     * in the normal flow. More precisely:
     *
     * * The static position for 'left' is the distance from the left edge of the
     *   containing block to the left margin edge of a hypothetical box that would
     *   have been the first box of the element if its 'position' property had
     *   been 'static' and 'float' had been 'none'. The value is negative if the
     *   hypothetical box is to the left of the containing block.
     * * The static position for 'right' is the distance from the right edge of the
     *   containing block to the right margin edge of the same hypothetical box as
     *   above. The value is positive if the hypothetical box is to the left of the
     *   containing block's edge.
     *
     * But rather than actually calculating the dimensions of that hypothetical box,
     * user agents are free to make a guess at its probable position.
     *
     * For the purposes of calculating the static position, the containing block of
     * fixed positioned elements is the initial containing block instead of the
     * viewport, and all scrollable boxes should be assumed to be scrolled to their
     * origin.
    \*---------------------------------------------------------------------------*/

    // see FIXME 2
    // Calculate the static distance if needed.
    computeInlineStaticDistance(logicalLeft, logicalRight, this, containerBlock, containerLogicalWidth, containerDirection);
    
    // Calculate constraint equation values for 'width' case.
    int logicalWidthResult;
    int logicalLeftResult;
    computePositionedLogicalWidthUsing(style()->logicalWidth(), containerBlock, containerDirection,
                                       containerLogicalWidth, bordersPlusPadding,
                                       logicalLeft, logicalRight, marginLogicalLeft, marginLogicalRight,
                                       logicalWidthResult, marginLogicalLeftAlias, marginLogicalRightAlias, logicalLeftResult);
    setLogicalWidth(logicalWidthResult);
    setLogicalLeft(logicalLeftResult);

    // Calculate constraint equation values for 'max-width' case.
    if (!style()->logicalMaxWidth().isUndefined()) {
        int maxLogicalWidth;
        int maxMarginLogicalLeft;
        int maxMarginLogicalRight;
        int maxLogicalLeftPos;

        computePositionedLogicalWidthUsing(style()->logicalMaxWidth(), containerBlock, containerDirection,
                                           containerLogicalWidth, bordersPlusPadding,
                                           logicalLeft, logicalRight, marginLogicalLeft, marginLogicalRight,
                                           maxLogicalWidth, maxMarginLogicalLeft, maxMarginLogicalRight, maxLogicalLeftPos);

        if (logicalWidth() > maxLogicalWidth) {
            setLogicalWidth(maxLogicalWidth);
            marginLogicalLeftAlias = maxMarginLogicalLeft;
            marginLogicalRightAlias = maxMarginLogicalRight;
            setLogicalLeft(maxLogicalLeftPos);
        }
    }

    // Calculate constraint equation values for 'min-width' case.
    if (!style()->logicalMinWidth().isZero()) {
        int minLogicalWidth;
        int minMarginLogicalLeft;
        int minMarginLogicalRight;
        int minLogicalLeftPos;

        computePositionedLogicalWidthUsing(style()->logicalMinWidth(), containerBlock, containerDirection,
                                           containerLogicalWidth, bordersPlusPadding,
                                           logicalLeft, logicalRight, marginLogicalLeft, marginLogicalRight,
                                           minLogicalWidth, minMarginLogicalLeft, minMarginLogicalRight, minLogicalLeftPos);

        if (logicalWidth() < minLogicalWidth) {
            setLogicalWidth(minLogicalWidth);
            marginLogicalLeftAlias = minMarginLogicalLeft;
            marginLogicalRightAlias = minMarginLogicalRight;
            setLogicalLeft(minLogicalLeftPos);
        }
    }

    if (stretchesToMinIntrinsicLogicalWidth() && logicalWidth() < minPreferredLogicalWidth() - bordersPlusPadding) {
        computePositionedLogicalWidthUsing(Length(minPreferredLogicalWidth() - bordersPlusPadding, Fixed), containerBlock, containerDirection,
                                           containerLogicalWidth, bordersPlusPadding,
                                           logicalLeft, logicalRight, marginLogicalLeft, marginLogicalRight,
                                           logicalWidthResult, marginLogicalLeftAlias, marginLogicalRightAlias, logicalLeftResult);
        setLogicalWidth(logicalWidthResult);
        setLogicalLeft(logicalLeftResult);
    }

    // Put logicalWidth() into correct form.
    setLogicalWidth(logicalWidth() + bordersPlusPadding);
}

static void computeLogicalLeftPositionedOffset(int& logicalLeftPos, const RenderBox* child, int logicalWidthValue, const RenderBoxModelObject* containerBlock, int containerLogicalWidth)
{
    // Deal with differing writing modes here.  Our offset needs to be in the containing block's coordinate space. If the containing block is flipped
    // along this axis, then we need to flip the coordinate.  This can only happen if the containing block is both a flipped mode and perpendicular to us.
    if (containerBlock->isHorizontalWritingMode() != child->isHorizontalWritingMode() && containerBlock->style()->isFlippedBlocksWritingMode()) {
        logicalLeftPos = containerLogicalWidth - logicalWidthValue - logicalLeftPos;
        logicalLeftPos += (child->isHorizontalWritingMode() ? containerBlock->borderRight() : containerBlock->borderBottom());
    } else
        logicalLeftPos += (child->isHorizontalWritingMode() ? containerBlock->borderLeft() : containerBlock->borderTop());
}

void RenderBox::computePositionedLogicalWidthUsing(Length logicalWidth, const RenderBoxModelObject* containerBlock, TextDirection containerDirection,
                                                   int containerLogicalWidth, int bordersPlusPadding,
                                                   Length logicalLeft, Length logicalRight, Length marginLogicalLeft, Length marginLogicalRight,
                                                   int& logicalWidthValue, int& marginLogicalLeftValue, int& marginLogicalRightValue, int& logicalLeftPos)
{
    // 'left' and 'right' cannot both be 'auto' because one would of been
    // converted to the static position already
    ASSERT(!(logicalLeft.isAuto() && logicalRight.isAuto()));

    int logicalLeftValue = 0;

    bool logicalWidthIsAuto = logicalWidth.isIntrinsicOrAuto();
    bool logicalLeftIsAuto = logicalLeft.isAuto();
    bool logicalRightIsAuto = logicalRight.isAuto();

    if (!logicalLeftIsAuto && !logicalWidthIsAuto && !logicalRightIsAuto) {
        /*-----------------------------------------------------------------------*\
         * If none of the three is 'auto': If both 'margin-left' and 'margin-
         * right' are 'auto', solve the equation under the extra constraint that
         * the two margins get equal values, unless this would make them negative,
         * in which case when direction of the containing block is 'ltr' ('rtl'),
         * set 'margin-left' ('margin-right') to zero and solve for 'margin-right'
         * ('margin-left'). If one of 'margin-left' or 'margin-right' is 'auto',
         * solve the equation for that value. If the values are over-constrained,
         * ignore the value for 'left' (in case the 'direction' property of the
         * containing block is 'rtl') or 'right' (in case 'direction' is 'ltr')
         * and solve for that value.
        \*-----------------------------------------------------------------------*/
        // NOTE:  It is not necessary to solve for 'right' in the over constrained
        // case because the value is not used for any further calculations.

        logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);
        logicalWidthValue = computeContentBoxLogicalWidth(logicalWidth.calcValue(containerLogicalWidth));

        const int availableSpace = containerLogicalWidth - (logicalLeftValue + logicalWidthValue + logicalRight.calcValue(containerLogicalWidth) + bordersPlusPadding);

        // Margins are now the only unknown
        if (marginLogicalLeft.isAuto() && marginLogicalRight.isAuto()) {
            // Both margins auto, solve for equality
            if (availableSpace >= 0) {
                marginLogicalLeftValue = availableSpace / 2; // split the difference
                marginLogicalRightValue = availableSpace - marginLogicalLeftValue; // account for odd valued differences
            } else {
                // see FIXME 1
                if (containerDirection == LTR) {
                    marginLogicalLeftValue = 0;
                    marginLogicalRightValue = availableSpace; // will be negative
                } else {
                    marginLogicalLeftValue = availableSpace; // will be negative
                    marginLogicalRightValue = 0;
                }
            }
        } else if (marginLogicalLeft.isAuto()) {
            // Solve for left margin
            marginLogicalRightValue = marginLogicalRight.calcValue(containerLogicalWidth);
            marginLogicalLeftValue = availableSpace - marginLogicalRightValue;
        } else if (marginLogicalRight.isAuto()) {
            // Solve for right margin
            marginLogicalLeftValue = marginLogicalLeft.calcValue(containerLogicalWidth);
            marginLogicalRightValue = availableSpace - marginLogicalLeftValue;
        } else {
            // Over-constrained, solve for left if direction is RTL
            marginLogicalLeftValue = marginLogicalLeft.calcValue(containerLogicalWidth);
            marginLogicalRightValue = marginLogicalRight.calcValue(containerLogicalWidth);

            // see FIXME 1 -- used to be "this->style()->direction()"
            if (containerDirection == RTL)
                logicalLeftValue = (availableSpace + logicalLeftValue) - marginLogicalLeftValue - marginLogicalRightValue;
        }
    } else {
        /*--------------------------------------------------------------------*\
         * Otherwise, set 'auto' values for 'margin-left' and 'margin-right'
         * to 0, and pick the one of the following six rules that applies.
         *
         * 1. 'left' and 'width' are 'auto' and 'right' is not 'auto', then the
         *    width is shrink-to-fit. Then solve for 'left'
         *
         *              OMIT RULE 2 AS IT SHOULD NEVER BE HIT
         * ------------------------------------------------------------------
         * 2. 'left' and 'right' are 'auto' and 'width' is not 'auto', then if
         *    the 'direction' property of the containing block is 'ltr' set
         *    'left' to the static position, otherwise set 'right' to the
         *    static position. Then solve for 'left' (if 'direction is 'rtl')
         *    or 'right' (if 'direction' is 'ltr').
         * ------------------------------------------------------------------
         *
         * 3. 'width' and 'right' are 'auto' and 'left' is not 'auto', then the
         *    width is shrink-to-fit . Then solve for 'right'
         * 4. 'left' is 'auto', 'width' and 'right' are not 'auto', then solve
         *    for 'left'
         * 5. 'width' is 'auto', 'left' and 'right' are not 'auto', then solve
         *    for 'width'
         * 6. 'right' is 'auto', 'left' and 'width' are not 'auto', then solve
         *    for 'right'
         *
         * Calculation of the shrink-to-fit width is similar to calculating the
         * width of a table cell using the automatic table layout algorithm.
         * Roughly: calculate the preferred width by formatting the content
         * without breaking lines other than where explicit line breaks occur,
         * and also calculate the preferred minimum width, e.g., by trying all
         * possible line breaks. CSS 2.1 does not define the exact algorithm.
         * Thirdly, calculate the available width: this is found by solving
         * for 'width' after setting 'left' (in case 1) or 'right' (in case 3)
         * to 0.
         *
         * Then the shrink-to-fit width is:
         * min(max(preferred minimum width, available width), preferred width).
        \*--------------------------------------------------------------------*/
        // NOTE: For rules 3 and 6 it is not necessary to solve for 'right'
        // because the value is not used for any further calculations.

        // Calculate margins, 'auto' margins are ignored.
        marginLogicalLeftValue = marginLogicalLeft.calcMinValue(containerLogicalWidth);
        marginLogicalRightValue = marginLogicalRight.calcMinValue(containerLogicalWidth);

        const int availableSpace = containerLogicalWidth - (marginLogicalLeftValue + marginLogicalRightValue + bordersPlusPadding);

        // FIXME: Is there a faster way to find the correct case?
        // Use rule/case that applies.
        if (logicalLeftIsAuto && logicalWidthIsAuto && !logicalRightIsAuto) {
            // RULE 1: (use shrink-to-fit for width, and solve of left)
            int logicalRightValue = logicalRight.calcValue(containerLogicalWidth);

            // FIXME: would it be better to have shrink-to-fit in one step?
            int preferredWidth = maxPreferredLogicalWidth() - bordersPlusPadding;
            int preferredMinWidth = minPreferredLogicalWidth() - bordersPlusPadding;
            int availableWidth = availableSpace - logicalRightValue;
            logicalWidthValue = min(max(preferredMinWidth, availableWidth), preferredWidth);
            logicalLeftValue = availableSpace - (logicalWidthValue + logicalRightValue);
        } else if (!logicalLeftIsAuto && logicalWidthIsAuto && logicalRightIsAuto) {
            // RULE 3: (use shrink-to-fit for width, and no need solve of right)
            logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);

            // FIXME: would it be better to have shrink-to-fit in one step?
            int preferredWidth = maxPreferredLogicalWidth() - bordersPlusPadding;
            int preferredMinWidth = minPreferredLogicalWidth() - bordersPlusPadding;
            int availableWidth = availableSpace - logicalLeftValue;
            logicalWidthValue = min(max(preferredMinWidth, availableWidth), preferredWidth);
        } else if (logicalLeftIsAuto && !logicalWidthIsAuto && !logicalRightIsAuto) {
            // RULE 4: (solve for left)
            logicalWidthValue = computeContentBoxLogicalWidth(logicalWidth.calcValue(containerLogicalWidth));
            logicalLeftValue = availableSpace - (logicalWidthValue + logicalRight.calcValue(containerLogicalWidth));
        } else if (!logicalLeftIsAuto && logicalWidthIsAuto && !logicalRightIsAuto) {
            // RULE 5: (solve for width)
            logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);
            logicalWidthValue = availableSpace - (logicalLeftValue + logicalRight.calcValue(containerLogicalWidth));
        } else if (!logicalLeftIsAuto && !logicalWidthIsAuto && logicalRightIsAuto) {
            // RULE 6: (no need solve for right)
            logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);
            logicalWidthValue = computeContentBoxLogicalWidth(logicalWidth.calcValue(containerLogicalWidth));
        }
    }

    // Use computed values to calculate the horizontal position.

    // FIXME: This hack is needed to calculate the  logical left position for a 'rtl' relatively
    // positioned, inline because right now, it is using the logical left position
    // of the first line box when really it should use the last line box.  When
    // this is fixed elsewhere, this block should be removed.
    if (containerBlock->isRenderInline() && !containerBlock->style()->isLeftToRightDirection()) {
        const RenderInline* flow = toRenderInline(containerBlock);
        InlineFlowBox* firstLine = flow->firstLineBox();
        InlineFlowBox* lastLine = flow->lastLineBox();
        if (firstLine && lastLine && firstLine != lastLine) {
            logicalLeftPos = logicalLeftValue + marginLogicalLeftValue + lastLine->borderLogicalLeft() + (lastLine->logicalLeft() - firstLine->logicalLeft());
            return;
        }
    }

    logicalLeftPos = logicalLeftValue + marginLogicalLeftValue;
    computeLogicalLeftPositionedOffset(logicalLeftPos, this, logicalWidthValue, containerBlock, containerLogicalWidth);
}

static void computeBlockStaticDistance(Length& logicalTop, Length& logicalBottom, const RenderBox* child, const RenderBoxModelObject* containerBlock)
{
    if (!logicalTop.isAuto() || !logicalBottom.isAuto())
        return;
    
    // FIXME: The static distance computation has not been patched for mixed writing modes.
    int staticLogicalTop = child->layer()->staticBlockPosition() - containerBlock->borderBefore();
    for (RenderObject* curr = child->parent(); curr && curr != containerBlock; curr = curr->container()) {
        if (curr->isBox() && !curr->isTableRow())
            staticLogicalTop += toRenderBox(curr)->logicalTop();
    }
    logicalTop.setValue(Fixed, staticLogicalTop);
}

void RenderBox::computePositionedLogicalHeight()
{
    if (isReplaced()) {
        computePositionedLogicalHeightReplaced();
        return;
    }

    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.6.4 "Absolutely positioned, non-replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-non-replaced-height>
    // (block-style-comments in this function and in computePositionedLogicalHeightUsing()
    // correspond to text from the spec)


    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    const RenderBoxModelObject* containerBlock = toRenderBoxModelObject(container());

    const int containerLogicalHeight = containingBlockLogicalHeightForPositioned(containerBlock);

    bool isHorizontal = isHorizontalWritingMode();
    bool isFlipped = style()->isFlippedBlocksWritingMode();
    const int bordersPlusPadding = borderAndPaddingLogicalHeight();
    const Length marginBefore = style()->marginBefore();
    const Length marginAfter = style()->marginAfter();
    int& marginBeforeAlias = isHorizontal ? (isFlipped ? m_marginBottom : m_marginTop) : (isFlipped ? m_marginRight: m_marginLeft);
    int& marginAfterAlias = isHorizontal ? (isFlipped ? m_marginTop : m_marginBottom) : (isFlipped ? m_marginLeft: m_marginRight);

    Length logicalTop = style()->logicalTop();
    Length logicalBottom = style()->logicalBottom();
        
    /*---------------------------------------------------------------------------*\
     * For the purposes of this section and the next, the term "static position"
     * (of an element) refers, roughly, to the position an element would have had
     * in the normal flow. More precisely, the static position for 'top' is the
     * distance from the top edge of the containing block to the top margin edge
     * of a hypothetical box that would have been the first box of the element if
     * its 'position' property had been 'static' and 'float' had been 'none'. The
     * value is negative if the hypothetical box is above the containing block.
     *
     * But rather than actually calculating the dimensions of that hypothetical
     * box, user agents are free to make a guess at its probable position.
     *
     * For the purposes of calculating the static position, the containing block
     * of fixed positioned elements is the initial containing block instead of
     * the viewport.
    \*---------------------------------------------------------------------------*/

    // see FIXME 2
    // Calculate the static distance if needed.
    computeBlockStaticDistance(logicalTop, logicalBottom, this, containerBlock);

    int logicalHeightResult; // Needed to compute overflow.
    int logicalTopPos;

    // Calculate constraint equation values for 'height' case.
    computePositionedLogicalHeightUsing(style()->logicalHeight(), containerBlock, containerLogicalHeight, bordersPlusPadding,
                                        logicalTop, logicalBottom, marginBefore, marginAfter,
                                        logicalHeightResult, marginBeforeAlias, marginAfterAlias, logicalTopPos);
    setLogicalTop(logicalTopPos);

    // Avoid doing any work in the common case (where the values of min-height and max-height are their defaults).
    // see FIXME 3

    // Calculate constraint equation values for 'max-height' case.
    if (!style()->logicalMaxHeight().isUndefined()) {
        int maxLogicalHeight;
        int maxMarginBefore;
        int maxMarginAfter;
        int maxLogicalTopPos;

        computePositionedLogicalHeightUsing(style()->logicalMaxHeight(), containerBlock, containerLogicalHeight, bordersPlusPadding,
                                            logicalTop, logicalBottom, marginBefore, marginAfter,
                                            maxLogicalHeight, maxMarginBefore, maxMarginAfter, maxLogicalTopPos);

        if (logicalHeightResult > maxLogicalHeight) {
            logicalHeightResult = maxLogicalHeight;
            marginBeforeAlias = maxMarginBefore;
            marginAfterAlias = maxMarginAfter;
            setLogicalTop(maxLogicalTopPos);
        }
    }

    // Calculate constraint equation values for 'min-height' case.
    if (!style()->logicalMinHeight().isZero()) {
        int minLogicalHeight;
        int minMarginBefore;
        int minMarginAfter;
        int minLogicalTopPos;

        computePositionedLogicalHeightUsing(style()->logicalMinHeight(), containerBlock, containerLogicalHeight, bordersPlusPadding,
                                            logicalTop, logicalBottom, marginBefore, marginAfter,
                                            minLogicalHeight, minMarginBefore, minMarginAfter, minLogicalTopPos);

        if (logicalHeightResult < minLogicalHeight) {
            logicalHeightResult = minLogicalHeight;
            marginBeforeAlias = minMarginBefore;
            marginAfterAlias = minMarginAfter;
            setLogicalTop(minLogicalTopPos);
        }
    }

    // Set final height value.
    setLogicalHeight(logicalHeightResult + bordersPlusPadding);
}

static void computeLogicalTopPositionedOffset(int& logicalTopPos, const RenderBox* child, int logicalHeightValue, const RenderBoxModelObject* containerBlock, int containerLogicalHeight)
{
    // Deal with differing writing modes here.  Our offset needs to be in the containing block's coordinate space. If the containing block is flipped
    // along this axis, then we need to flip the coordinate.  This can only happen if the containing block is both a flipped mode and perpendicular to us.
    if ((child->style()->isFlippedBlocksWritingMode() && child->isHorizontalWritingMode() != containerBlock->isHorizontalWritingMode())
        || (child->style()->isFlippedBlocksWritingMode() != containerBlock->style()->isFlippedBlocksWritingMode() && child->isHorizontalWritingMode() == containerBlock->isHorizontalWritingMode()))
        logicalTopPos = containerLogicalHeight - logicalHeightValue - logicalTopPos;

    // Our offset is from the logical bottom edge in a flipped environment, e.g., right for vertical-rl and bottom for horizontal-bt.
    if (containerBlock->style()->isFlippedBlocksWritingMode() && child->isHorizontalWritingMode() == containerBlock->isHorizontalWritingMode()) {
        if (child->isHorizontalWritingMode())
            logicalTopPos += containerBlock->borderBottom();
        else
            logicalTopPos += containerBlock->borderRight();
    } else {
        if (child->isHorizontalWritingMode())
            logicalTopPos += containerBlock->borderTop();
        else
            logicalTopPos += containerBlock->borderLeft();
    }
}

void RenderBox::computePositionedLogicalHeightUsing(Length logicalHeightLength, const RenderBoxModelObject* containerBlock,
                                                    int containerLogicalHeight, int bordersPlusPadding,
                                                    Length logicalTop, Length logicalBottom, Length marginBefore, Length marginAfter,
                                                    int& logicalHeightValue, int& marginBeforeValue, int& marginAfterValue, int& logicalTopPos)
{
    // 'top' and 'bottom' cannot both be 'auto' because 'top would of been
    // converted to the static position in computePositionedLogicalHeight()
    ASSERT(!(logicalTop.isAuto() && logicalBottom.isAuto()));

    int contentLogicalHeight = logicalHeight() - bordersPlusPadding;

    int logicalTopValue = 0;

    bool logicalHeightIsAuto = logicalHeightLength.isAuto();
    bool logicalTopIsAuto = logicalTop.isAuto();
    bool logicalBottomIsAuto = logicalBottom.isAuto();

    // Height is never unsolved for tables.
    if (isTable()) {
        logicalHeightLength.setValue(Fixed, contentLogicalHeight);
        logicalHeightIsAuto = false;
    }

    if (!logicalTopIsAuto && !logicalHeightIsAuto && !logicalBottomIsAuto) {
        /*-----------------------------------------------------------------------*\
         * If none of the three are 'auto': If both 'margin-top' and 'margin-
         * bottom' are 'auto', solve the equation under the extra constraint that
         * the two margins get equal values. If one of 'margin-top' or 'margin-
         * bottom' is 'auto', solve the equation for that value. If the values
         * are over-constrained, ignore the value for 'bottom' and solve for that
         * value.
        \*-----------------------------------------------------------------------*/
        // NOTE:  It is not necessary to solve for 'bottom' in the over constrained
        // case because the value is not used for any further calculations.

        logicalHeightValue = computeContentBoxLogicalHeight(logicalHeightLength.calcValue(containerLogicalHeight));
        logicalTopValue = logicalTop.calcValue(containerLogicalHeight);

        const int availableSpace = containerLogicalHeight - (logicalTopValue + logicalHeightValue + logicalBottom.calcValue(containerLogicalHeight) + bordersPlusPadding);

        // Margins are now the only unknown
        if (marginBefore.isAuto() && marginAfter.isAuto()) {
            // Both margins auto, solve for equality
            // NOTE: This may result in negative values.
            marginBeforeValue = availableSpace / 2; // split the difference
            marginAfterValue = availableSpace - marginBeforeValue; // account for odd valued differences
        } else if (marginBefore.isAuto()) {
            // Solve for top margin
            marginAfterValue = marginAfter.calcValue(containerLogicalHeight);
            marginBeforeValue = availableSpace - marginAfterValue;
        } else if (marginAfter.isAuto()) {
            // Solve for bottom margin
            marginBeforeValue = marginBefore.calcValue(containerLogicalHeight);
            marginAfterValue = availableSpace - marginBeforeValue;
        } else {
            // Over-constrained, (no need solve for bottom)
            marginBeforeValue = marginBefore.calcValue(containerLogicalHeight);
            marginAfterValue = marginAfter.calcValue(containerLogicalHeight);
        }
    } else {
        /*--------------------------------------------------------------------*\
         * Otherwise, set 'auto' values for 'margin-top' and 'margin-bottom'
         * to 0, and pick the one of the following six rules that applies.
         *
         * 1. 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then
         *    the height is based on the content, and solve for 'top'.
         *
         *              OMIT RULE 2 AS IT SHOULD NEVER BE HIT
         * ------------------------------------------------------------------
         * 2. 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then
         *    set 'top' to the static position, and solve for 'bottom'.
         * ------------------------------------------------------------------
         *
         * 3. 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then
         *    the height is based on the content, and solve for 'bottom'.
         * 4. 'top' is 'auto', 'height' and 'bottom' are not 'auto', and
         *    solve for 'top'.
         * 5. 'height' is 'auto', 'top' and 'bottom' are not 'auto', and
         *    solve for 'height'.
         * 6. 'bottom' is 'auto', 'top' and 'height' are not 'auto', and
         *    solve for 'bottom'.
        \*--------------------------------------------------------------------*/
        // NOTE: For rules 3 and 6 it is not necessary to solve for 'bottom'
        // because the value is not used for any further calculations.

        // Calculate margins, 'auto' margins are ignored.
        marginBeforeValue = marginBefore.calcMinValue(containerLogicalHeight);
        marginAfterValue = marginAfter.calcMinValue(containerLogicalHeight);

        const int availableSpace = containerLogicalHeight - (marginBeforeValue + marginAfterValue + bordersPlusPadding);

        // Use rule/case that applies.
        if (logicalTopIsAuto && logicalHeightIsAuto && !logicalBottomIsAuto) {
            // RULE 1: (height is content based, solve of top)
            logicalHeightValue = contentLogicalHeight;
            logicalTopValue = availableSpace - (logicalHeightValue + logicalBottom.calcValue(containerLogicalHeight));
        } else if (!logicalTopIsAuto && logicalHeightIsAuto && logicalBottomIsAuto) {
            // RULE 3: (height is content based, no need solve of bottom)
            logicalTopValue = logicalTop.calcValue(containerLogicalHeight);
            logicalHeightValue = contentLogicalHeight;
        } else if (logicalTopIsAuto && !logicalHeightIsAuto && !logicalBottomIsAuto) {
            // RULE 4: (solve of top)
            logicalHeightValue = computeContentBoxLogicalHeight(logicalHeightLength.calcValue(containerLogicalHeight));
            logicalTopValue = availableSpace - (logicalHeightValue + logicalBottom.calcValue(containerLogicalHeight));
        } else if (!logicalTopIsAuto && logicalHeightIsAuto && !logicalBottomIsAuto) {
            // RULE 5: (solve of height)
            logicalTopValue = logicalTop.calcValue(containerLogicalHeight);
            logicalHeightValue = max(0, availableSpace - (logicalTopValue + logicalBottom.calcValue(containerLogicalHeight)));
        } else if (!logicalTopIsAuto && !logicalHeightIsAuto && logicalBottomIsAuto) {
            // RULE 6: (no need solve of bottom)
            logicalHeightValue = computeContentBoxLogicalHeight(logicalHeightLength.calcValue(containerLogicalHeight));
            logicalTopValue = logicalTop.calcValue(containerLogicalHeight);
        }
    }

    // Use computed values to calculate the vertical position.
    logicalTopPos = logicalTopValue + marginBeforeValue;
    computeLogicalTopPositionedOffset(logicalTopPos, this, logicalHeightValue, containerBlock, containerLogicalHeight);
}

void RenderBox::computePositionedLogicalWidthReplaced()
{
    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.3.8 "Absolutely positioned, replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-replaced-width>
    // (block-style-comments in this function correspond to text from the spec and
    // the numbers correspond to numbers in spec)

    // We don't use containingBlock(), since we may be positioned by an enclosing
    // relative positioned inline.
    const RenderBoxModelObject* containerBlock = toRenderBoxModelObject(container());

    const int containerLogicalWidth = containingBlockLogicalWidthForPositioned(containerBlock);

    // To match WinIE, in quirks mode use the parent's 'direction' property
    // instead of the the container block's.
    TextDirection containerDirection = (document()->inQuirksMode()) ? parent()->style()->direction() : containerBlock->style()->direction();

    // Variables to solve.
    bool isHorizontal = isHorizontalWritingMode();
    Length logicalLeft = style()->logicalLeft();
    Length logicalRight = style()->logicalRight();
    Length marginLogicalLeft = isHorizontal ? style()->marginLeft() : style()->marginTop();
    Length marginLogicalRight = isHorizontal ? style()->marginRight() : style()->marginBottom();
    int& marginLogicalLeftAlias = isHorizontal ? m_marginLeft : m_marginTop;
    int& marginLogicalRightAlias = isHorizontal ? m_marginRight : m_marginBottom;

    /*-----------------------------------------------------------------------*\
     * 1. The used value of 'width' is determined as for inline replaced
     *    elements.
    \*-----------------------------------------------------------------------*/
    // NOTE: This value of width is FINAL in that the min/max width calculations
    // are dealt with in computeReplacedWidth().  This means that the steps to produce
    // correct max/min in the non-replaced version, are not necessary.
    setLogicalWidth(computeReplacedLogicalWidth() + borderAndPaddingLogicalWidth());
    const int availableSpace = containerLogicalWidth - logicalWidth();

    /*-----------------------------------------------------------------------*\
     * 2. If both 'left' and 'right' have the value 'auto', then if 'direction'
     *    of the containing block is 'ltr', set 'left' to the static position;
     *    else if 'direction' is 'rtl', set 'right' to the static position.
    \*-----------------------------------------------------------------------*/
    // see FIXME 2
    computeInlineStaticDistance(logicalLeft, logicalRight, this, containerBlock, containerLogicalWidth, containerDirection);

    /*-----------------------------------------------------------------------*\
     * 3. If 'left' or 'right' are 'auto', replace any 'auto' on 'margin-left'
     *    or 'margin-right' with '0'.
    \*-----------------------------------------------------------------------*/
    if (logicalLeft.isAuto() || logicalRight.isAuto()) {
        if (marginLogicalLeft.isAuto())
            marginLogicalLeft.setValue(Fixed, 0);
        if (marginLogicalRight.isAuto())
            marginLogicalRight.setValue(Fixed, 0);
    }

    /*-----------------------------------------------------------------------*\
     * 4. If at this point both 'margin-left' and 'margin-right' are still
     *    'auto', solve the equation under the extra constraint that the two
     *    margins must get equal values, unless this would make them negative,
     *    in which case when the direction of the containing block is 'ltr'
     *    ('rtl'), set 'margin-left' ('margin-right') to zero and solve for
     *    'margin-right' ('margin-left').
    \*-----------------------------------------------------------------------*/
    int logicalLeftValue = 0;
    int logicalRightValue = 0;

    if (marginLogicalLeft.isAuto() && marginLogicalRight.isAuto()) {
        // 'left' and 'right' cannot be 'auto' due to step 3
        ASSERT(!(logicalLeft.isAuto() && logicalRight.isAuto()));

        logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);
        logicalRightValue = logicalRight.calcValue(containerLogicalWidth);

        int difference = availableSpace - (logicalLeftValue + logicalRightValue);
        if (difference > 0) {
            marginLogicalLeftAlias = difference / 2; // split the difference
            marginLogicalRightAlias = difference - marginLogicalLeftAlias; // account for odd valued differences
        } else {
            // see FIXME 1
            if (containerDirection == LTR) {
                marginLogicalLeftAlias = 0;
                marginLogicalRightAlias = difference; // will be negative
            } else {
                marginLogicalLeftAlias = difference; // will be negative
                marginLogicalRightAlias = 0;
            }
        }

    /*-----------------------------------------------------------------------*\
     * 5. If at this point there is an 'auto' left, solve the equation for
     *    that value.
    \*-----------------------------------------------------------------------*/
    } else if (logicalLeft.isAuto()) {
        marginLogicalLeftAlias = marginLogicalLeft.calcValue(containerLogicalWidth);
        marginLogicalRightAlias = marginLogicalRight.calcValue(containerLogicalWidth);
        logicalRightValue = logicalRight.calcValue(containerLogicalWidth);

        // Solve for 'left'
        logicalLeftValue = availableSpace - (logicalRightValue + marginLogicalLeftAlias + marginLogicalRightAlias);
    } else if (logicalRight.isAuto()) {
        marginLogicalLeftAlias = marginLogicalLeft.calcValue(containerLogicalWidth);
        marginLogicalRightAlias = marginLogicalRight.calcValue(containerLogicalWidth);
        logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);

        // Solve for 'right'
        logicalRightValue = availableSpace - (logicalLeftValue + marginLogicalLeftAlias + marginLogicalRightAlias);
    } else if (marginLogicalLeft.isAuto()) {
        marginLogicalRightAlias = marginLogicalRight.calcValue(containerLogicalWidth);
        logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);
        logicalRightValue = logicalRight.calcValue(containerLogicalWidth);

        // Solve for 'margin-left'
        marginLogicalLeftAlias = availableSpace - (logicalLeftValue + logicalRightValue + marginLogicalRightAlias);
    } else if (marginLogicalRight.isAuto()) {
        marginLogicalLeftAlias = marginLogicalLeft.calcValue(containerLogicalWidth);
        logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);
        logicalRightValue = logicalRight.calcValue(containerLogicalWidth);

        // Solve for 'margin-right'
        marginLogicalRightAlias = availableSpace - (logicalLeftValue + logicalRightValue + marginLogicalLeftAlias);
    } else {
        // Nothing is 'auto', just calculate the values.
        marginLogicalLeftAlias = marginLogicalLeft.calcValue(containerLogicalWidth);
        marginLogicalRightAlias = marginLogicalRight.calcValue(containerLogicalWidth);
        logicalRightValue = logicalRight.calcValue(containerLogicalWidth);
        logicalLeftValue = logicalLeft.calcValue(containerLogicalWidth);
    }

    /*-----------------------------------------------------------------------*\
     * 6. If at this point the values are over-constrained, ignore the value
     *    for either 'left' (in case the 'direction' property of the
     *    containing block is 'rtl') or 'right' (in case 'direction' is
     *    'ltr') and solve for that value.
    \*-----------------------------------------------------------------------*/
    // NOTE:  It is not necessary to solve for 'right' when the direction is
    // LTR because the value is not used.
    int totalLogicalWidth = logicalWidth() + logicalLeftValue + logicalRightValue +  marginLogicalLeftAlias + marginLogicalRightAlias;
    if (totalLogicalWidth > containerLogicalWidth && (containerDirection == RTL))
        logicalLeftValue = containerLogicalWidth - (totalLogicalWidth - logicalLeftValue);

    // FIXME: Deal with differing writing modes here.  Our offset needs to be in the containing block's coordinate space, so that
    // can make the result here rather complicated to compute.
    
    // Use computed values to calculate the horizontal position.

    // FIXME: This hack is needed to calculate the logical left position for a 'rtl' relatively
    // positioned, inline containing block because right now, it is using the logical left position
    // of the first line box when really it should use the last line box.  When
    // this is fixed elsewhere, this block should be removed.
    if (containerBlock->isRenderInline() && !containerBlock->style()->isLeftToRightDirection()) {
        const RenderInline* flow = toRenderInline(containerBlock);
        InlineFlowBox* firstLine = flow->firstLineBox();
        InlineFlowBox* lastLine = flow->lastLineBox();
        if (firstLine && lastLine && firstLine != lastLine) {
            setLogicalLeft(logicalLeftValue + marginLogicalLeftAlias + lastLine->borderLogicalLeft() + (lastLine->logicalLeft() - firstLine->logicalLeft()));
            return;
        }
    }

    int logicalLeftPos = logicalLeftValue + marginLogicalLeftAlias;
    computeLogicalLeftPositionedOffset(logicalLeftPos, this, logicalWidth(), containerBlock, containerLogicalWidth);    
    setLogicalLeft(logicalLeftPos);
}

void RenderBox::computePositionedLogicalHeightReplaced()
{
    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.6.5 "Absolutely positioned, replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-replaced-height>
    // (block-style-comments in this function correspond to text from the spec and
    // the numbers correspond to numbers in spec)

    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    const RenderBoxModelObject* containerBlock = toRenderBoxModelObject(container());

    const int containerLogicalHeight = containingBlockLogicalHeightForPositioned(containerBlock);

    // Variables to solve.
    bool isHorizontal = isHorizontalWritingMode();
    bool isFlipped = style()->isFlippedBlocksWritingMode();
    Length marginBefore = style()->marginBefore();
    Length marginAfter = style()->marginAfter();
    int& marginBeforeAlias = isHorizontal ? (isFlipped ? m_marginBottom : m_marginTop) : (isFlipped ? m_marginRight: m_marginLeft);
    int& marginAfterAlias = isHorizontal ? (isFlipped ? m_marginTop : m_marginBottom) : (isFlipped ? m_marginLeft: m_marginRight);

    Length logicalTop = style()->logicalTop();
    Length logicalBottom = style()->logicalBottom();

    /*-----------------------------------------------------------------------*\
     * 1. The used value of 'height' is determined as for inline replaced
     *    elements.
    \*-----------------------------------------------------------------------*/
    // NOTE: This value of height is FINAL in that the min/max height calculations
    // are dealt with in computeReplacedHeight().  This means that the steps to produce
    // correct max/min in the non-replaced version, are not necessary.
    setLogicalHeight(computeReplacedLogicalHeight() + borderAndPaddingLogicalHeight());
    const int availableSpace = containerLogicalHeight - logicalHeight();

    /*-----------------------------------------------------------------------*\
     * 2. If both 'top' and 'bottom' have the value 'auto', replace 'top'
     *    with the element's static position.
    \*-----------------------------------------------------------------------*/
    // see FIXME 2
    computeBlockStaticDistance(logicalTop, logicalBottom, this, containerBlock);

    /*-----------------------------------------------------------------------*\
     * 3. If 'bottom' is 'auto', replace any 'auto' on 'margin-top' or
     *    'margin-bottom' with '0'.
    \*-----------------------------------------------------------------------*/
    // FIXME: The spec. says that this step should only be taken when bottom is
    // auto, but if only top is auto, this makes step 4 impossible.
    if (logicalTop.isAuto() || logicalBottom.isAuto()) {
        if (marginBefore.isAuto())
            marginBefore.setValue(Fixed, 0);
        if (marginAfter.isAuto())
            marginAfter.setValue(Fixed, 0);
    }

    /*-----------------------------------------------------------------------*\
     * 4. If at this point both 'margin-top' and 'margin-bottom' are still
     *    'auto', solve the equation under the extra constraint that the two
     *    margins must get equal values.
    \*-----------------------------------------------------------------------*/
    int logicalTopValue = 0;
    int logicalBottomValue = 0;

    if (marginBefore.isAuto() && marginAfter.isAuto()) {
        // 'top' and 'bottom' cannot be 'auto' due to step 2 and 3 combined.
        ASSERT(!(logicalTop.isAuto() || logicalBottom.isAuto()));

        logicalTopValue = logicalTop.calcValue(containerLogicalHeight);
        logicalBottomValue = logicalBottom.calcValue(containerLogicalHeight);

        int difference = availableSpace - (logicalTopValue + logicalBottomValue);
        // NOTE: This may result in negative values.
        marginBeforeAlias =  difference / 2; // split the difference
        marginAfterAlias = difference - marginBeforeAlias; // account for odd valued differences

    /*-----------------------------------------------------------------------*\
     * 5. If at this point there is only one 'auto' left, solve the equation
     *    for that value.
    \*-----------------------------------------------------------------------*/
    } else if (logicalTop.isAuto()) {
        marginBeforeAlias = marginBefore.calcValue(containerLogicalHeight);
        marginAfterAlias = marginAfter.calcValue(containerLogicalHeight);
        logicalBottomValue = logicalBottom.calcValue(containerLogicalHeight);

        // Solve for 'top'
        logicalTopValue = availableSpace - (logicalBottomValue + marginBeforeAlias + marginAfterAlias);
    } else if (logicalBottom.isAuto()) {
        marginBeforeAlias = marginBefore.calcValue(containerLogicalHeight);
        marginAfterAlias = marginAfter.calcValue(containerLogicalHeight);
        logicalTopValue = logicalTop.calcValue(containerLogicalHeight);

        // Solve for 'bottom'
        // NOTE: It is not necessary to solve for 'bottom' because we don't ever
        // use the value.
    } else if (marginBefore.isAuto()) {
        marginAfterAlias = marginAfter.calcValue(containerLogicalHeight);
        logicalTopValue = logicalTop.calcValue(containerLogicalHeight);
        logicalBottomValue = logicalBottom.calcValue(containerLogicalHeight);

        // Solve for 'margin-top'
        marginBeforeAlias = availableSpace - (logicalTopValue + logicalBottomValue + marginAfterAlias);
    } else if (marginAfter.isAuto()) {
        marginBeforeAlias = marginBefore.calcValue(containerLogicalHeight);
        logicalTopValue = logicalTop.calcValue(containerLogicalHeight);
        logicalBottomValue = logicalBottom.calcValue(containerLogicalHeight);

        // Solve for 'margin-bottom'
        marginAfterAlias = availableSpace - (logicalTopValue + logicalBottomValue + marginBeforeAlias);
    } else {
        // Nothing is 'auto', just calculate the values.
        marginBeforeAlias = marginBefore.calcValue(containerLogicalHeight);
        marginAfterAlias = marginAfter.calcValue(containerLogicalHeight);
        logicalTopValue = logicalTop.calcValue(containerLogicalHeight);
        // NOTE: It is not necessary to solve for 'bottom' because we don't ever
        // use the value.
     }

    /*-----------------------------------------------------------------------*\
     * 6. If at this point the values are over-constrained, ignore the value
     *    for 'bottom' and solve for that value.
    \*-----------------------------------------------------------------------*/
    // NOTE: It is not necessary to do this step because we don't end up using
    // the value of 'bottom' regardless of whether the values are over-constrained
    // or not.

    // Use computed values to calculate the vertical position.
    int logicalTopPos = logicalTopValue + marginBeforeAlias;
    computeLogicalTopPositionedOffset(logicalTopPos, this, logicalHeight(), containerBlock, containerLogicalHeight);
    setLogicalTop(logicalTopPos);
}

IntRect RenderBox::localCaretRect(InlineBox* box, int caretOffset, int* extraWidthToEndOfLine)
{
    // VisiblePositions at offsets inside containers either a) refer to the positions before/after
    // those containers (tables and select elements) or b) refer to the position inside an empty block.
    // They never refer to children.
    // FIXME: Paint the carets inside empty blocks differently than the carets before/after elements.

    // FIXME: What about border and padding?
    IntRect rect(x(), y(), caretWidth, height());
    bool ltr = box ? box->isLeftToRightDirection() : style()->isLeftToRightDirection();

    if ((!caretOffset) ^ ltr)
        rect.move(IntSize(width() - caretWidth, 0));

    if (box) {
        RootInlineBox* rootBox = box->root();
        int top = rootBox->lineTop();
        rect.setY(top);
        rect.setHeight(rootBox->lineBottom() - top);
    }

    // If height of box is smaller than font height, use the latter one,
    // otherwise the caret might become invisible.
    //
    // Also, if the box is not a replaced element, always use the font height.
    // This prevents the "big caret" bug described in:
    // <rdar://problem/3777804> Deleting all content in a document can result in giant tall-as-window insertion point
    //
    // FIXME: ignoring :first-line, missing good reason to take care of
    int fontHeight = style()->fontMetrics().height();
    if (fontHeight > rect.height() || (!isReplaced() && !isTable()))
        rect.setHeight(fontHeight);

    if (extraWidthToEndOfLine)
        *extraWidthToEndOfLine = x() + width() - rect.maxX();

    // Move to local coords
    rect.move(-x(), -y());
    return rect;
}

VisiblePosition RenderBox::positionForPoint(const IntPoint& point)
{
    // no children...return this render object's element, if there is one, and offset 0
    if (!firstChild())
        return createVisiblePosition(node() ? firstPositionInOrBeforeNode(node()) : Position(0, 0));

    int xPos = point.x();
    int yPos = point.y();

    if (isTable() && node()) {
        int right = contentWidth() + borderAndPaddingWidth();
        int bottom = contentHeight() + borderAndPaddingHeight();
        
        if (xPos < 0 || xPos > right || yPos < 0 || yPos > bottom) {
            if (xPos <= right / 2)
                return createVisiblePosition(firstPositionInOrBeforeNode(node()));
            return createVisiblePosition(lastPositionInOrAfterNode(node()));
        }
    }

    // Pass off to the closest child.
    int minDist = INT_MAX;
    RenderBox* closestRenderer = 0;
    int newX = xPos;
    int newY = yPos;
    if (isTableRow()) {
        newX += x();
        newY += y();
    }
    for (RenderObject* renderObject = firstChild(); renderObject; renderObject = renderObject->nextSibling()) {
        if ((!renderObject->firstChild() && !renderObject->isInline() && !renderObject->isBlockFlow() )
            || renderObject->style()->visibility() != VISIBLE)
            continue;
        
        if (!renderObject->isBox())
            continue;
        
        RenderBox* renderer = toRenderBox(renderObject);

        int top = renderer->borderTop() + renderer->paddingTop() + (isTableRow() ? 0 : renderer->y());
        int bottom = top + renderer->contentHeight();
        int left = renderer->borderLeft() + renderer->paddingLeft() + (isTableRow() ? 0 : renderer->x());
        int right = left + renderer->contentWidth();
        
        if (xPos <= right && xPos >= left && yPos <= top && yPos >= bottom) {
            if (renderer->isTableRow())
                return renderer->positionForCoordinates(xPos + newX - renderer->x(), yPos + newY - renderer->y());
            return renderer->positionForCoordinates(xPos - renderer->x(), yPos - renderer->y());
        }

        // Find the distance from (x, y) to the box.  Split the space around the box into 8 pieces
        // and use a different compare depending on which piece (x, y) is in.
        IntPoint cmp;
        if (xPos > right) {
            if (yPos < top)
                cmp = IntPoint(right, top);
            else if (yPos > bottom)
                cmp = IntPoint(right, bottom);
            else
                cmp = IntPoint(right, yPos);
        } else if (xPos < left) {
            if (yPos < top)
                cmp = IntPoint(left, top);
            else if (yPos > bottom)
                cmp = IntPoint(left, bottom);
            else
                cmp = IntPoint(left, yPos);
        } else {
            if (yPos < top)
                cmp = IntPoint(xPos, top);
            else
                cmp = IntPoint(xPos, bottom);
        }
        
        int x1minusx2 = cmp.x() - xPos;
        int y1minusy2 = cmp.y() - yPos;
        
        int dist = x1minusx2 * x1minusx2 + y1minusy2 * y1minusy2;
        if (dist < minDist) {
            closestRenderer = renderer;
            minDist = dist;
        }
    }
    
    if (closestRenderer)
        return closestRenderer->positionForCoordinates(newX - closestRenderer->x(), newY - closestRenderer->y());
    
    return createVisiblePosition(firstPositionInOrBeforeNode(node()));
}

bool RenderBox::shrinkToAvoidFloats() const
{
    // Floating objects don't shrink.  Objects that don't avoid floats don't shrink.  Marquees don't shrink.
    if ((isInline() && !isHTMLMarquee()) || !avoidsFloats() || isFloating())
        return false;

    // All auto-width objects that avoid floats should always use lineWidth.
    return style()->width().isAuto(); 
}

bool RenderBox::avoidsFloats() const
{
    return isReplaced() || hasOverflowClip() || isHR() || isLegend() || isWritingModeRoot() || isDeprecatedFlexItem();
}

void RenderBox::addShadowOverflow()
{
    int shadowLeft;
    int shadowRight;
    int shadowTop;
    int shadowBottom;
    style()->getBoxShadowExtent(shadowTop, shadowRight, shadowBottom, shadowLeft);
    IntRect borderBox = borderBoxRect();
    int overflowLeft = borderBox.x() + shadowLeft;
    int overflowRight = borderBox.maxX() + shadowRight;
    int overflowTop = borderBox.y() + shadowTop;
    int overflowBottom = borderBox.maxY() + shadowBottom;
    addVisualOverflow(IntRect(overflowLeft, overflowTop, overflowRight - overflowLeft, overflowBottom - overflowTop));
}

void RenderBox::addOverflowFromChild(RenderBox* child, const IntSize& delta)
{
    // Only propagate layout overflow from the child if the child isn't clipping its overflow.  If it is, then
    // its overflow is internal to it, and we don't care about it.  layoutOverflowRectForPropagation takes care of this
    // and just propagates the border box rect instead.
    IntRect childLayoutOverflowRect = child->layoutOverflowRectForPropagation(style());
    childLayoutOverflowRect.move(delta);
    addLayoutOverflow(childLayoutOverflowRect);
            
    // Add in visual overflow from the child.  Even if the child clips its overflow, it may still
    // have visual overflow of its own set from box shadows or reflections.  It is unnecessary to propagate this
    // overflow if we are clipping our own overflow.
    if (child->hasSelfPaintingLayer() || hasOverflowClip())
        return;
    IntRect childVisualOverflowRect = child->visualOverflowRectForPropagation(style());
    childVisualOverflowRect.move(delta);
    addVisualOverflow(childVisualOverflowRect);
}

void RenderBox::addLayoutOverflow(const IntRect& rect)
{
    IntRect clientBox = clientBoxRect();
    if (clientBox.contains(rect) || rect.isEmpty())
        return;
    
    // For overflow clip objects, we don't want to propagate overflow into unreachable areas.
    IntRect overflowRect(rect);
    if (hasOverflowClip() || isRenderView()) {
        // Overflow is in the block's coordinate space and thus is flipped for horizontal-bt and vertical-rl 
        // writing modes.  At this stage that is actually a simplification, since we can treat horizontal-tb/bt as the same
        // and vertical-lr/rl as the same.
        bool hasTopOverflow = !style()->isLeftToRightDirection() && !isHorizontalWritingMode();
        bool hasLeftOverflow = !style()->isLeftToRightDirection() && isHorizontalWritingMode();
        
        if (!hasTopOverflow)
            overflowRect.shiftYEdgeTo(max(overflowRect.y(), clientBox.y()));
        else
            overflowRect.shiftMaxYEdgeTo(min(overflowRect.maxY(), clientBox.maxY()));
        if (!hasLeftOverflow)
            overflowRect.shiftXEdgeTo(max(overflowRect.x(), clientBox.x()));
        else
            overflowRect.shiftMaxXEdgeTo(min(overflowRect.maxX(), clientBox.maxX()));
        
        // Now re-test with the adjusted rectangle and see if it has become unreachable or fully
        // contained.
        if (clientBox.contains(overflowRect) || overflowRect.isEmpty())
            return;
    }

    if (!m_overflow)
        m_overflow = adoptPtr(new RenderOverflow(clientBox, borderBoxRect()));
    
    m_overflow->addLayoutOverflow(overflowRect);
}

void RenderBox::addVisualOverflow(const IntRect& rect)
{
    IntRect borderBox = borderBoxRect();
    if (borderBox.contains(rect) || rect.isEmpty())
        return;
        
    if (!m_overflow)
        m_overflow = adoptPtr(new RenderOverflow(clientBoxRect(), borderBox));
    
    m_overflow->addVisualOverflow(rect);
}

void RenderBox::clearLayoutOverflow()
{
    if (!m_overflow)
        return;
    
    if (visualOverflowRect() == borderBoxRect()) {
        m_overflow.clear();
        return;
    }
    
    m_overflow->resetLayoutOverflow(borderBoxRect());
}

int RenderBox::lineHeight(bool /*firstLine*/, LineDirectionMode direction, LinePositionMode /*linePositionMode*/) const
{
    if (isReplaced())
        return direction == HorizontalLine ? m_marginTop + height() + m_marginBottom : m_marginRight + width() + m_marginLeft;
    return 0;
}

int RenderBox::baselinePosition(FontBaseline baselineType, bool /*firstLine*/, LineDirectionMode direction, LinePositionMode /*linePositionMode*/) const
{
    if (isReplaced()) {
        int result = direction == HorizontalLine ? m_marginTop + height() + m_marginBottom : m_marginRight + width() + m_marginLeft;
        if (baselineType == AlphabeticBaseline)
            return result;
        return result - result / 2;
    }
    return 0;
}


RenderLayer* RenderBox::enclosingFloatPaintingLayer() const
{
    const RenderObject* curr = this;
    while (curr) {
        RenderLayer* layer = curr->hasLayer() && curr->isBox() ? toRenderBoxModelObject(curr)->layer() : 0;
        if (layer && layer->isSelfPaintingLayer())
            return layer;
        curr = curr->parent();
    }
    return 0;
}

IntRect RenderBox::logicalVisualOverflowRectForPropagation(RenderStyle* parentStyle) const
{
    IntRect rect = visualOverflowRectForPropagation(parentStyle);
    if (!parentStyle->isHorizontalWritingMode())
        return rect.transposedRect();
    return rect;
}

IntRect RenderBox::visualOverflowRectForPropagation(RenderStyle* parentStyle) const
{
    // If the writing modes of the child and parent match, then we don't have to 
    // do anything fancy. Just return the result.
    IntRect rect = visualOverflowRect();
    if (parentStyle->writingMode() == style()->writingMode())
        return rect;
    
    // We are putting ourselves into our parent's coordinate space.  If there is a flipped block mismatch
    // in a particular axis, then we have to flip the rect along that axis.
    if (style()->writingMode() == RightToLeftWritingMode || parentStyle->writingMode() == RightToLeftWritingMode)
        rect.setX(width() - rect.maxX());
    else if (style()->writingMode() == BottomToTopWritingMode || parentStyle->writingMode() == BottomToTopWritingMode)
        rect.setY(height() - rect.maxY());

    return rect;
}

IntRect RenderBox::logicalLayoutOverflowRectForPropagation(RenderStyle* parentStyle) const
{
    IntRect rect = layoutOverflowRectForPropagation(parentStyle);
    if (!parentStyle->isHorizontalWritingMode())
        return rect.transposedRect();
    return rect;
}

IntRect RenderBox::layoutOverflowRectForPropagation(RenderStyle* parentStyle) const
{
    // Only propagate interior layout overflow if we don't clip it.
    IntRect rect = borderBoxRect();
    if (!hasOverflowClip())
        rect.unite(layoutOverflowRect());

    bool hasTransform = hasLayer() && layer()->transform();
    if (isRelPositioned() || hasTransform) {
        // If we are relatively positioned or if we have a transform, then we have to convert
        // this rectangle into physical coordinates, apply relative positioning and transforms
        // to it, and then convert it back.
        flipForWritingMode(rect);
        
        if (hasTransform)
            rect = layer()->currentTransform().mapRect(rect);

        if (isRelPositioned())
            rect.move(relativePositionOffsetX(), relativePositionOffsetY());
        
        // Now we need to flip back.
        flipForWritingMode(rect);
    }
    
    // If the writing modes of the child and parent match, then we don't have to 
    // do anything fancy. Just return the result.
    if (parentStyle->writingMode() == style()->writingMode())
        return rect;
    
    // We are putting ourselves into our parent's coordinate space.  If there is a flipped block mismatch
    // in a particular axis, then we have to flip the rect along that axis.
    if (style()->writingMode() == RightToLeftWritingMode || parentStyle->writingMode() == RightToLeftWritingMode)
        rect.setX(width() - rect.maxX());
    else if (style()->writingMode() == BottomToTopWritingMode || parentStyle->writingMode() == BottomToTopWritingMode)
        rect.setY(height() - rect.maxY());

    return rect;
}

IntPoint RenderBox::flipForWritingMode(const RenderBox* child, const IntPoint& point, FlippingAdjustment adjustment) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return point;
    
    // The child is going to add in its x() and y(), so we have to make sure it ends up in
    // the right place.
    if (isHorizontalWritingMode())
        return IntPoint(point.x(), point.y() + height() - child->height() - child->y() - (adjustment == ParentToChildFlippingAdjustment ? child->y() : 0));
    return IntPoint(point.x() + width() - child->width() - child->x() - (adjustment == ParentToChildFlippingAdjustment ? child->x() : 0), point.y());
}

void RenderBox::flipForWritingMode(IntRect& rect) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return;

    if (isHorizontalWritingMode())
        rect.setY(height() - rect.maxY());
    else
        rect.setX(width() - rect.maxX());
}

int RenderBox::flipForWritingMode(int position) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return position;
    return logicalHeight() - position;
}

IntPoint RenderBox::flipForWritingMode(const IntPoint& position) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return position;
    return isHorizontalWritingMode() ? IntPoint(position.x(), height() - position.y()) : IntPoint(width() - position.x(), position.y());
}

IntPoint RenderBox::flipForWritingModeIncludingColumns(const IntPoint& point) const
{
    if (!hasColumns() || !style()->isFlippedBlocksWritingMode())
        return flipForWritingMode(point);
    return toRenderBlock(this)->flipForWritingModeIncludingColumns(point);
}

IntSize RenderBox::flipForWritingMode(const IntSize& offset) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return offset;
    return isHorizontalWritingMode() ? IntSize(offset.width(), height() - offset.height()) : IntSize(width() - offset.width(), offset.height());
}

FloatPoint RenderBox::flipForWritingMode(const FloatPoint& position) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return position;
    return isHorizontalWritingMode() ? FloatPoint(position.x(), height() - position.y()) : FloatPoint(width() - position.x(), position.y());
}

void RenderBox::flipForWritingMode(FloatRect& rect) const
{
    if (!style()->isFlippedBlocksWritingMode())
        return;

    if (isHorizontalWritingMode())
        rect.setY(height() - rect.maxY());
    else
        rect.setX(width() - rect.maxX());
}

IntSize RenderBox::locationOffsetIncludingFlipping() const
{
    RenderBlock* containerBlock = containingBlock();
    if (!containerBlock || containerBlock == this)
        return locationOffset();
    
    IntRect rect(frameRect());
    containerBlock->flipForWritingMode(rect); // FIXME: This is wrong if we are an absolutely positioned object enclosed by a relative-positioned inline.
    return IntSize(rect.x(), rect.y());
}

} // namespace WebCore
