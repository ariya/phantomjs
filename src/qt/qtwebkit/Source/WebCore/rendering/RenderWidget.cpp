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
#include "Frame.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "RenderCounter.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "RenderWidgetProtector.h"
#include <wtf/StackStats.h>

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

unsigned WidgetHierarchyUpdatesSuspensionScope::s_widgetHierarchyUpdateSuspendCount = 0;

WidgetHierarchyUpdatesSuspensionScope::WidgetToParentMap& WidgetHierarchyUpdatesSuspensionScope::widgetNewParentMap()
{
    DEFINE_STATIC_LOCAL(WidgetToParentMap, map, ());
    return map;
}

void WidgetHierarchyUpdatesSuspensionScope::moveWidgets()
{
    WidgetToParentMap map;
    widgetNewParentMap().swap(map);
    WidgetToParentMap::iterator end = map.end();
    for (WidgetToParentMap::iterator it = map.begin(); it != end; ++it) {
        Widget* child = it->key.get();
        ScrollView* currentParent = child->parent();
        FrameView* newParent = it->value;
        if (newParent != currentParent) {
            if (currentParent)
                currentParent->removeChild(child);
            if (newParent)
                newParent->addChild(child);
        }
    }
}

static void moveWidgetToParentSoon(Widget* child, FrameView* parent)
{
    if (!WidgetHierarchyUpdatesSuspensionScope::isSuspended()) {
        if (parent)
            parent->addChild(child);
        else
            child->removeFromParent();
        return;
    }
    WidgetHierarchyUpdatesSuspensionScope::scheduleWidgetToMove(child, parent);
}

RenderWidget::RenderWidget(Element* element)
    : RenderReplaced(element)
    , m_widget(0)
    , m_frameView(element->document()->view())
    // Reference counting is used to prevent the widget from being
    // destroyed while inside the Widget code, which might not be
    // able to handle that.
    , m_refCount(1)
{
    view()->addWidget(this);
}

void RenderWidget::willBeDestroyed()
{
    if (RenderView* v = view())
        v->removeWidget(this);
    
    if (AXObjectCache* cache = document()->existingAXObjectCache()) {
        cache->childrenChanged(this->parent());
        cache->remove(this);
    }

    setWidget(0);

    RenderReplaced::willBeDestroyed();
}

void RenderWidget::destroy()
{
    willBeDestroyed();

    // Grab the arena from node()->document()->renderArena() before clearing the node pointer.
    // Clear the node before deref-ing, as this may be deleted when deref is called.
    RenderArena* arena = renderArena();
    clearNode();
    deref(arena);
}

RenderWidget::~RenderWidget()
{
    ASSERT(m_refCount <= 0);
    clearWidget();
}

// Widgets are always placed on integer boundaries, so rounding the size is actually
// the desired behavior. This function is here because it's otherwise seldom what we
// want to do with a LayoutRect.
static inline IntRect roundedIntRect(const LayoutRect& rect)
{
    return IntRect(roundedIntPoint(rect.location()), roundedIntSize(rect.size()));
}

bool RenderWidget::setWidgetGeometry(const LayoutRect& frame)
{
    if (!node())
        return false;

    IntRect clipRect = roundedIntRect(enclosingLayer()->childrenClipRect());
    IntRect newFrame = roundedIntRect(frame);
    bool clipChanged = m_clipRect != clipRect;
    bool boundsChanged = m_widget->frameRect() != newFrame;

    if (!boundsChanged && !clipChanged)
        return false;

    m_clipRect = clipRect;

    RenderWidgetProtector protector(this);
    RefPtr<Node> protectedNode(node());
    m_widget->setFrameRect(newFrame);

    if (clipChanged && !boundsChanged)
        m_widget->clipRectChanged();
    
#if USE(ACCELERATED_COMPOSITING)
    if (hasLayer() && layer()->isComposited())
        layer()->backing()->updateAfterWidgetResize();
#endif
    
    return boundsChanged;
}

bool RenderWidget::updateWidgetGeometry()
{
    LayoutRect contentBox = contentBoxRect();
    if (!m_widget->transformsAffectFrameRect())
        return setWidgetGeometry(absoluteContentBox());

    LayoutRect absoluteContentBox(localToAbsoluteQuad(FloatQuad(contentBox)).boundingBox());
    if (m_widget->isFrameView()) {
        contentBox.setLocation(absoluteContentBox.location());
        return setWidgetGeometry(contentBox);
    }

    return setWidgetGeometry(absoluteContentBox);
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
                updateWidgetGeometry();

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
    StackStats::LayoutCheckPoint layoutCheckPoint;
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

void RenderWidget::notifyWidget(WidgetNotification notification)
{
    if (m_widget)
        m_widget->notifyWidget(notification);
}

void RenderWidget::paintContents(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    LayoutPoint adjustedPaintOffset = paintOffset + location();

    // Tell the widget to paint now. This is the only time the widget is allowed
    // to paint itself. That way it will composite properly with z-indexed layers.
    IntPoint widgetLocation = m_widget->frameRect().location();
    IntPoint paintLocation(roundToInt(adjustedPaintOffset.x() + borderLeft() + paddingLeft()),
        roundToInt(adjustedPaintOffset.y() + borderTop() + paddingTop()));
    IntRect paintRect = paintInfo.rect;

    IntSize widgetPaintOffset = paintLocation - widgetLocation;
    // When painting widgets into compositing layers, tx and ty are relative to the enclosing compositing layer,
    // not the root. In this case, shift the CTM and adjust the paintRect to be root-relative to fix plug-in drawing.
    if (!widgetPaintOffset.isZero()) {
        paintInfo.context->translate(widgetPaintOffset);
        paintRect.move(-widgetPaintOffset);
    }
    m_widget->paint(paintInfo.context, paintRect);

    if (!widgetPaintOffset.isZero())
        paintInfo.context->translate(-widgetPaintOffset);

    if (m_widget->isFrameView()) {
        FrameView* frameView = toFrameView(m_widget.get());
        bool runOverlapTests = !frameView->useSlowRepaintsIfNotOverlapped() || frameView->hasCompositedContentIncludingDescendants();
        if (paintInfo.overlapTestRequests && runOverlapTests) {
            ASSERT(!paintInfo.overlapTestRequests->contains(this));
            paintInfo.overlapTestRequests->set(this, m_widget->frameRect());
        }
    }
}

void RenderWidget::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!shouldPaint(paintInfo, paintOffset))
        return;

    LayoutPoint adjustedPaintOffset = paintOffset + location();

    if (hasBoxDecorations() && (paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection))
        paintBoxDecorations(paintInfo, adjustedPaintOffset);

    if (paintInfo.phase == PaintPhaseMask) {
        paintMask(paintInfo, adjustedPaintOffset);
        return;
    }

    if ((paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) && hasOutline())
        paintOutline(paintInfo, LayoutRect(adjustedPaintOffset, size()));

    if (!m_frameView || paintInfo.phase != PaintPhaseForeground)
        return;

#if PLATFORM(MAC)
    if (style()->highlight() != nullAtom && !paintInfo.context->paintingDisabled())
        paintCustomHighlight(paintOffset, style()->highlight(), true);
#endif

    if (style()->hasBorderRadius()) {
        LayoutRect borderRect = LayoutRect(adjustedPaintOffset, size());

        if (borderRect.isEmpty())
            return;

        // Push a clip if we have a border radius, since we want to round the foreground content that gets painted.
        paintInfo.context->save();
        RoundedRect roundedInnerRect = style()->getRoundedInnerBorderFor(borderRect,
            paddingTop() + borderTop(), paddingBottom() + borderBottom(), paddingLeft() + borderLeft(), paddingRight() + borderRight(), true, true);
        clipRoundedInnerRect(paintInfo.context, borderRect, roundedInnerRect);
    }

    if (m_widget)
        paintContents(paintInfo, paintOffset);

    if (style()->hasBorderRadius())
        paintInfo.context->restore();

    // Paint a partially transparent wash over selected widgets.
    if (isSelected() && !document()->printing()) {
        // FIXME: selectionRect() is in absolute, not painting coordinates.
        paintInfo.context->fillRect(pixelSnappedIntRect(selectionRect()), selectionBackgroundColor(), style()->colorSpace());
    }

    if (hasLayer() && layer()->canResize())
        layer()->paintResizer(paintInfo.context, roundedIntPoint(adjustedPaintOffset), paintInfo.rect);
}

void RenderWidget::setOverlapTestResult(bool isOverlapped)
{
    ASSERT(m_widget);
    ASSERT(m_widget->isFrameView());
    toFrameView(m_widget.get())->setIsOverlapped(isOverlapped);
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

    bool boundsChanged = updateWidgetGeometry();
    
    // if the frame bounds got changed, or if view needs layout (possibly indicating
    // content size is wrong) we have to do a layout to set the right widget size
    if (m_widget && m_widget->isFrameView()) {
        FrameView* frameView = toFrameView(m_widget.get());
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
    // The selection state for our containing block hierarchy is updated by the base class call.
    RenderReplaced::setSelectionState(state);

    if (m_widget)
        m_widget->setIsSelected(isSelected());
}

void RenderWidget::clearWidget()
{
    m_widget = 0;
}

RenderWidget* RenderWidget::find(const Widget* widget)
{
    return widgetRendererMap().get(widget);
}

bool RenderWidget::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction action)
{
    bool hadResult = result.innerNode();
    bool inside = RenderReplaced::nodeAtPoint(request, result, locationInContainer, accumulatedOffset, action);

    // Check to see if we are really over the widget itself (and not just in the border/padding area).
    if ((inside || result.isRectBasedTest()) && !hadResult && result.innerNode() == node())
        result.setIsOverWidget(contentBoxRect().contains(result.localPoint()));
    return inside;
}

CursorDirective RenderWidget::getCursor(const LayoutPoint& point, Cursor& cursor) const
{
    if (widget() && widget()->isPluginViewBase()) {
        // A plug-in is responsible for setting the cursor when the pointer is over it.
        return DoNotSetCursor;
    }
    return RenderReplaced::getCursor(point, cursor);
}

} // namespace WebCore
