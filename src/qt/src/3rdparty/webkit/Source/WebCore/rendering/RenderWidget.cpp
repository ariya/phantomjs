/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2009, 2010 Apple Inc. All rights reserved.
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
#include "RenderWidget.h"

#include "AXObjectCache.h"
#include "AnimationController.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "RenderCounter.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "RenderWidgetProtector.h"

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerBacking.h"
#endif

using namespace std;

namespace WebCore {

static HashMap<const Widget*, RenderWidget*>& widgetRendererMap()
{
    static HashMap<const Widget*, RenderWidget*>* staticWidgetRendererMap = new HashMap<const Widget*, RenderWidget*>;
    return *staticWidgetRendererMap;
}

static size_t widgetHierarchyUpdateSuspendCount;

typedef HashMap<RefPtr<Widget>, FrameView*> WidgetToParentMap;

static WidgetToParentMap& widgetNewParentMap()
{
    DEFINE_STATIC_LOCAL(WidgetToParentMap, map, ());
    return map;
}

void RenderWidget::suspendWidgetHierarchyUpdates()
{
    widgetHierarchyUpdateSuspendCount++;
}

void RenderWidget::resumeWidgetHierarchyUpdates()
{
    ASSERT(widgetHierarchyUpdateSuspendCount);
    if (widgetHierarchyUpdateSuspendCount == 1) {
        WidgetToParentMap map = widgetNewParentMap();
        widgetNewParentMap().clear();
        WidgetToParentMap::iterator end = map.end();
        for (WidgetToParentMap::iterator it = map.begin(); it != end; ++it) {
            Widget* child = it->first.get();
            ScrollView* currentParent = child->parent();
            FrameView* newParent = it->second;
            if (newParent != currentParent) {
                if (currentParent)
                    currentParent->removeChild(child);
                if (newParent)
                    newParent->addChild(child);
            }
        }
    }
    widgetHierarchyUpdateSuspendCount--;
}

static void moveWidgetToParentSoon(Widget* child, FrameView* parent)
{
    if (!widgetHierarchyUpdateSuspendCount) {
        if (parent)
            parent->addChild(child);
        else
            child->removeFromParent();
        return;
    }
    widgetNewParentMap().set(child, parent);
}

RenderWidget::RenderWidget(Node* node)
    : RenderReplaced(node)
    , m_widget(0)
    , m_frameView(node->document()->view())
    // Reference counting is used to prevent the widget from being
    // destroyed while inside the Widget code, which might not be
    // able to handle that.
    , m_refCount(1)
{
    view()->addWidget(this);
}

void RenderWidget::destroy()
{
    // We can't call the base class's destroy because we don't
    // want to unconditionally delete ourselves (we're ref-counted).
    // So the code below includes copied and pasted contents of
    // both RenderBox::destroy() and RenderObject::destroy().
    // Fix originally made for <rdar://problem/4228818>.

    animation()->cancelAnimations(this);

    if (RenderView* v = view())
        v->removeWidget(this);

    
    if (AXObjectCache::accessibilityEnabled()) {
        document()->axObjectCache()->childrenChanged(this->parent());
        document()->axObjectCache()->remove(this);
    }

    if (!documentBeingDestroyed() && parent()) 
        parent()->dirtyLinesFromChangedChild(this);

    remove();

    if (m_hasCounterNodeMap)
        RenderCounter::destroyCounterNodes(this);

    setWidget(0);

    // removes from override size map
    if (hasOverrideSize())
        setOverrideSize(-1);

    if (style() && (style()->logicalHeight().isPercent() || style()->logicalMinHeight().isPercent() || style()->logicalMaxHeight().isPercent()))
        RenderBlock::removePercentHeightDescendant(this);

    // If this renderer is owning renderer for the frameview's custom scrollbars,
    // we need to clear it from the scrollbar. See webkit bug 64737.
    if (style() && style()->hasPseudoStyle(SCROLLBAR) && frame() && frame()->view())
        frame()->view()->clearOwningRendererForCustomScrollbars(this);

    if (hasLayer()) {
        layer()->clearClipRects();
        setHasLayer(false);
        destroyLayer();
    }

    // Grab the arena from node()->document()->renderArena() before clearing the node pointer.
    // Clear the node before deref-ing, as this may be deleted when deref is called.
    RenderArena* arena = renderArena();
    setNode(0);
    deref(arena);
}

RenderWidget::~RenderWidget()
{
    ASSERT(m_refCount <= 0);
    clearWidget();
}

bool RenderWidget::setWidgetGeometry(const IntRect& frame, const IntSize& boundsSize)
{
    if (!node())
        return false;

    IntRect clipRect = enclosingLayer()->childrenClipRect();
    bool clipChanged = m_clipRect != clipRect;
    bool boundsChanged = m_widget->frameRect() != frame;

    if (!boundsChanged && !clipChanged)
        return false;

    m_clipRect = clipRect;

    RenderWidgetProtector protector(this);
    RefPtr<Node> protectedNode(node());
    m_widget->setFrameRect(frame);
    if (m_widget) // setFrameRect can run arbitrary script, which might clear m_widget.
        m_widget->setBoundsSize(boundsSize);
    
#if USE(ACCELERATED_COMPOSITING)
    if (hasLayer() && layer()->isComposited())
        layer()->backing()->updateAfterWidgetResize();
#endif
    
    return boundsChanged;
}

void RenderWidget::setWidget(PassRefPtr<Widget> widget)
{
    if (widget == m_widget)
        return;

    if (m_widget) {
        moveWidgetToParentSoon(m_widget.get(), 0);
        widgetRendererMap().remove(m_widget.get());
        clearWidget();
    }
    m_widget = widget;
    if (m_widget) {
        widgetRendererMap().add(m_widget.get(), this);
        // If we've already received a layout, apply the calculated space to the
        // widget immediately, but we have to have really been fully constructed (with a non-null
        // style pointer).
        if (style()) {
            if (!needsLayout())
                setWidgetGeometry(IntRect(localToAbsoluteQuad(FloatQuad(contentBoxRect())).boundingBox()), contentBoxRect().size());
            if (style()->visibility() != VISIBLE)
                m_widget->hide();
            else {
                m_widget->show();
                repaint();
            }
        }
        moveWidgetToParentSoon(m_widget.get(), m_frameView);
    }
}

void RenderWidget::layout()
{
    ASSERT(needsLayout());

    setNeedsLayout(false);
}

void RenderWidget::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderReplaced::styleDidChange(diff, oldStyle);
    if (m_widget) {
        if (style()->visibility() != VISIBLE)
            m_widget->hide();
        else
            m_widget->show();
    }
}

void RenderWidget::showSubstituteImage(PassRefPtr<Image> prpImage)
{
    m_substituteImage = prpImage;
    repaint();
}

void RenderWidget::notifyWidget(WidgetNotification notification)
{
    if (m_widget)
        m_widget->notifyWidget(notification);
}

void RenderWidget::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (!shouldPaint(paintInfo, tx, ty))
        return;

    tx += x();
    ty += y();

    if (hasBoxDecorations() && (paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection))
        paintBoxDecorations(paintInfo, tx, ty);

    if (paintInfo.phase == PaintPhaseMask) {
        paintMask(paintInfo, tx, ty);
        return;
    }

    if (!m_frameView || paintInfo.phase != PaintPhaseForeground || style()->visibility() != VISIBLE)
        return;

#if PLATFORM(MAC)
    if (style()->highlight() != nullAtom && !paintInfo.context->paintingDisabled())
        paintCustomHighlight(tx - x(), ty - y(), style()->highlight(), true);
#endif

    if (style()->hasBorderRadius()) {
        IntRect borderRect = IntRect(tx, ty, width(), height());

        if (borderRect.isEmpty())
            return;

        // Push a clip if we have a border radius, since we want to round the foreground content that gets painted.
        paintInfo.context->save();
        paintInfo.context->addRoundedRectClip(style()->getRoundedBorderFor(borderRect));
    }

    if (m_widget) {
        // Tell the widget to paint now.  This is the only time the widget is allowed
        // to paint itself.  That way it will composite properly with z-indexed layers.
        if (m_substituteImage)
            paintInfo.context->drawImage(m_substituteImage.get(), style()->colorSpace(), m_widget->frameRect());
        else {
            IntPoint widgetLocation = m_widget->frameRect().location();
            IntPoint paintLocation(tx + borderLeft() + paddingLeft(), ty + borderTop() + paddingTop());
            IntRect paintRect = paintInfo.rect;

            IntSize paintOffset = paintLocation - widgetLocation;
            // When painting widgets into compositing layers, tx and ty are relative to the enclosing compositing layer,
            // not the root. In this case, shift the CTM and adjust the paintRect to be root-relative to fix plug-in drawing.
            if (!paintOffset.isZero()) {
                paintInfo.context->translate(paintOffset);
                paintRect.move(-paintOffset);
            }
            m_widget->paint(paintInfo.context, paintRect);

            if (!paintOffset.isZero())
                paintInfo.context->translate(-paintOffset);
        }

        if (m_widget->isFrameView()) {
            FrameView* frameView = static_cast<FrameView*>(m_widget.get());
            bool runOverlapTests = !frameView->useSlowRepaintsIfNotOverlapped() || frameView->hasCompositedContentIncludingDescendants();
            if (paintInfo.overlapTestRequests && runOverlapTests) {
                ASSERT(!paintInfo.overlapTestRequests->contains(this));
                paintInfo.overlapTestRequests->set(this, m_widget->frameRect());
            }
         }
    }

    if (style()->hasBorderRadius())
        paintInfo.context->restore();

    // Paint a partially transparent wash over selected widgets.
    if (isSelected() && !document()->printing()) {
        // FIXME: selectionRect() is in absolute, not painting coordinates.
        paintInfo.context->fillRect(selectionRect(), selectionBackgroundColor(), style()->colorSpace());
    }
}

void RenderWidget::setOverlapTestResult(bool isOverlapped)
{
    ASSERT(m_widget);
    ASSERT(m_widget->isFrameView());
    static_cast<FrameView*>(m_widget.get())->setIsOverlapped(isOverlapped);
}

void RenderWidget::deref(RenderArena *arena)
{
    if (--m_refCount <= 0)
        arenaDelete(arena, this);
}

void RenderWidget::updateWidgetPosition()
{
    if (!m_widget || !node()) // Check the node in case destroy() has been called.
        return;

    IntRect contentBox = contentBoxRect();
    IntRect absoluteContentBox = IntRect(localToAbsoluteQuad(FloatQuad(contentBox)).boundingBox());
    bool boundsChanged = setWidgetGeometry(absoluteContentBox, contentBox.size());

    // if the frame bounds got changed, or if view needs layout (possibly indicating
    // content size is wrong) we have to do a layout to set the right widget size
    if (m_widget && m_widget->isFrameView()) {
        FrameView* frameView = static_cast<FrameView*>(m_widget.get());
        // Check the frame's page to make sure that the frame isn't in the process of being destroyed.
        if ((boundsChanged || frameView->needsLayout()) && frameView->frame()->page())
            frameView->layout();
    }
}

void RenderWidget::widgetPositionsUpdated()
{
    if (!m_widget)
        return;
    m_widget->widgetPositionsUpdated();
}

IntRect RenderWidget::windowClipRect() const
{
    if (!m_frameView)
        return IntRect();

    return intersection(m_frameView->contentsToWindow(m_clipRect), m_frameView->windowClipRect());
}

void RenderWidget::setSelectionState(SelectionState state)
{
    if (selectionState() != state) {
        RenderReplaced::setSelectionState(state);
        if (m_widget)
            m_widget->setIsSelected(isSelected());
    }
}

void RenderWidget::clearWidget()
{
    m_widget = 0;
}

RenderWidget* RenderWidget::find(const Widget* widget)
{
    return widgetRendererMap().get(widget);
}

bool RenderWidget::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, HitTestAction action)
{
    bool hadResult = result.innerNode();
    bool inside = RenderReplaced::nodeAtPoint(request, result, x, y, tx, ty, action);
    
    // Check to see if we are really over the widget itself (and not just in the border/padding area).
    if ((inside || result.isRectBasedTest()) && !hadResult && result.innerNode() == node())
        result.setIsOverWidget(contentBoxRect().contains(result.localPoint()));
    return inside;
}

} // namespace WebCore
