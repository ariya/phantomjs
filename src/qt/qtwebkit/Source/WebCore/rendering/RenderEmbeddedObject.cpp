/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "RenderEmbeddedObject.h"

#include "CSSValueKeywords.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Cursor.h"
#include "EventHandler.h"
#include "Font.h"
#include "FontSelector.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "GraphicsContext.h"
#include "HTMLEmbedElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "HTMLParamElement.h"
#include "HTMLPlugInElement.h"
#include "HitTestResult.h"
#include "LocalizedStrings.h"
#include "MIMETypeRegistry.h"
#include "MouseEvent.h"
#include "Page.h"
#include "PaintInfo.h"
#include "Path.h"
#include "PlatformMouseEvent.h"
#include "PluginViewBase.h"
#include "RenderLayer.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidgetProtector.h"
#include "Settings.h"
#include "Text.h"
#include "TextRun.h"
#include <wtf/StackStats.h>

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#include "HTMLMediaElement.h"
#endif

namespace WebCore {

using namespace HTMLNames;

static const float replacementTextRoundedRectHeight = 18;
static const float replacementTextRoundedRectLeftRightTextMargin = 6;
static const float replacementTextRoundedRectBottomTextPadding = 1;
static const float replacementTextRoundedRectOpacity = 0.8f;
static const float replacementTextRoundedRectRadius = 5;
static const float replacementArrowLeftMargin = 5;
static const float replacementArrowPadding = 4;

static const Color& replacementTextRoundedRectPressedColor()
{
    static const Color pressed(205, 205, 205);
    return pressed;
}

static const Color& replacementTextRoundedRectColor()
{
    static const Color standard(221, 221, 221);
    return standard;
}

static const Color& replacementTextColor()
{
    static const Color standard(102, 102, 102);
    return standard;
}

RenderEmbeddedObject::RenderEmbeddedObject(Element* element)
    : RenderPart(element)
    , m_hasFallbackContent(false)
    , m_isPluginUnavailable(false)
    , m_isUnavailablePluginIndicatorHidden(false)
    , m_unavailablePluginIndicatorIsPressed(false)
    , m_mouseDownWasInUnavailablePluginIndicator(false)
{
    // Actual size is not known yet, report the default intrinsic size.
    view()->frameView()->incrementVisuallyNonEmptyPixelCount(roundedIntSize(intrinsicSize()));
}

RenderEmbeddedObject::~RenderEmbeddedObject()
{
    if (frameView())
        frameView()->removeWidgetToUpdate(this);
}

#if USE(ACCELERATED_COMPOSITING)
bool RenderEmbeddedObject::requiresLayer() const
{
    if (RenderPart::requiresLayer())
        return true;
    
    return allowsAcceleratedCompositing();
}

bool RenderEmbeddedObject::allowsAcceleratedCompositing() const
{
    return widget() && widget()->isPluginViewBase() && toPluginViewBase(widget())->platformLayer();
}
#endif

static String unavailablePluginReplacementText(RenderEmbeddedObject::PluginUnavailabilityReason pluginUnavailabilityReason)
{
    switch (pluginUnavailabilityReason) {
    case RenderEmbeddedObject::PluginMissing:
        return missingPluginText();
    case RenderEmbeddedObject::PluginCrashed:
        return crashedPluginText();
    case RenderEmbeddedObject::PluginBlockedByContentSecurityPolicy:
        return blockedPluginByContentSecurityPolicyText();
    case RenderEmbeddedObject::InsecurePluginVersion:
        return insecurePluginVersionText();
    }

    ASSERT_NOT_REACHED();
    return String();
}

static bool shouldUnavailablePluginMessageBeButton(Document* document, RenderEmbeddedObject::PluginUnavailabilityReason pluginUnavailabilityReason)
{
    Page* page = document->page();
    return page && page->chrome().client()->shouldUnavailablePluginMessageBeButton(pluginUnavailabilityReason);
}

void RenderEmbeddedObject::setPluginUnavailabilityReason(PluginUnavailabilityReason pluginUnavailabilityReason)
{
    setPluginUnavailabilityReasonWithDescription(pluginUnavailabilityReason, unavailablePluginReplacementText(pluginUnavailabilityReason));
}

void RenderEmbeddedObject::setPluginUnavailabilityReasonWithDescription(PluginUnavailabilityReason pluginUnavailabilityReason, const String& description)
{
    ASSERT(!m_isPluginUnavailable);
    m_isPluginUnavailable = true;
    m_pluginUnavailabilityReason = pluginUnavailabilityReason;

    if (description.isEmpty())
        m_unavailablePluginReplacementText = unavailablePluginReplacementText(pluginUnavailabilityReason);
    else
        m_unavailablePluginReplacementText = description;
}

void RenderEmbeddedObject::setUnavailablePluginIndicatorIsPressed(bool pressed)
{
    if (m_unavailablePluginIndicatorIsPressed == pressed)
        return;

    m_unavailablePluginIndicatorIsPressed = pressed;
    repaint();
}

void RenderEmbeddedObject::paintSnapshotImage(PaintInfo& paintInfo, const LayoutPoint& paintOffset, Image* image)
{
    LayoutUnit cWidth = contentWidth();
    LayoutUnit cHeight = contentHeight();
    if (!cWidth || !cHeight)
        return;

    GraphicsContext* context = paintInfo.context;
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

void RenderEmbeddedObject::paintContents(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    Element* element = toElement(node());
    if (!element || !element->isPluginElement())
        return;

    HTMLPlugInElement* plugInElement = toHTMLPlugInElement(element);

    if (plugInElement->displayState() > HTMLPlugInElement::DisplayingSnapshot) {
        RenderPart::paintContents(paintInfo, paintOffset);
        if (!plugInElement->isRestartedPlugin())
            return;
    }

    if (!plugInElement->isPlugInImageElement())
        return;

    Image* snapshot = toHTMLPlugInImageElement(plugInElement)->snapshotImage();
    if (snapshot)
        paintSnapshotImage(paintInfo, paintOffset, snapshot);
}

void RenderEmbeddedObject::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    Page* page = 0;
    if (Frame* frame = this->frame())
        page = frame->page();

    if (isPluginUnavailable()) {
        if (page && paintInfo.phase == PaintPhaseForeground)
            page->addRelevantUnpaintedObject(this, visualOverflowRect());
        RenderReplaced::paint(paintInfo, paintOffset);
        return;
    }

    if (page && paintInfo.phase == PaintPhaseForeground)
        page->addRelevantRepaintedObject(this, visualOverflowRect());

    RenderPart::paint(paintInfo, paintOffset);
}

void RenderEmbeddedObject::paintReplaced(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!showsUnavailablePluginIndicator())
        return;

    if (paintInfo.phase == PaintPhaseSelection)
        return;

    GraphicsContext* context = paintInfo.context;
    if (context->paintingDisabled())
        return;

    FloatRect contentRect;
    Path path;
    FloatRect replacementTextRect;
    FloatRect arrowRect;
    Font font;
    TextRun run("");
    float textWidth;
    if (!getReplacementTextGeometry(paintOffset, contentRect, path, replacementTextRect, arrowRect, font, run, textWidth))
        return;

    GraphicsContextStateSaver stateSaver(*context);
    context->clip(contentRect);
    context->setAlpha(replacementTextRoundedRectOpacity);
    context->setFillColor(m_unavailablePluginIndicatorIsPressed ? replacementTextRoundedRectPressedColor() : replacementTextRoundedRectColor(), style()->colorSpace());
    context->fillPath(path);

    const FontMetrics& fontMetrics = font.fontMetrics();
    float labelX = roundf(replacementTextRect.location().x() + (replacementTextRect.size().width() - textWidth) / 2);
    float labelY = roundf(replacementTextRect.location().y() + (replacementTextRect.size().height() - fontMetrics.height()) / 2 + fontMetrics.ascent());
    context->setFillColor(replacementTextColor(), style()->colorSpace());
    context->drawBidiText(font, run, FloatPoint(labelX, labelY));
}

void RenderEmbeddedObject::setUnavailablePluginIndicatorIsHidden(bool hidden)
{
    m_isUnavailablePluginIndicatorHidden = hidden;

    repaint();
}

static void addReplacementArrowPath(Path& path, const FloatRect& insideRect)
{
    FloatRect rect(insideRect);
    rect.inflate(-replacementArrowPadding);

    FloatPoint center = rect.center();
    FloatSize arrowEdge(rect.width() / 2, rect.height() / 3);
    FloatSize arrowHorizontalEdge(arrowEdge.width(), 0);
    FloatSize arrowVerticalEdge(0, arrowEdge.height());

    path.moveTo(FloatPoint(floorf(center.x()), floorf(rect.y())));
    path.addLineTo(FloatPoint(floorf(center.x()), floorf(rect.y() + arrowEdge.height())));
    path.addLineTo(FloatPoint(ceilf(center.x() - arrowEdge.width()), floorf(rect.y() + arrowEdge.height())));
    path.addLineTo(FloatPoint(ceilf(center.x() - arrowEdge.width()), ceilf(rect.y() + arrowEdge.height() + arrowEdge.height())));
    path.addLineTo(FloatPoint(floorf(center.x()), ceilf(rect.y() + arrowEdge.height() + arrowEdge.height())));
    path.addLineTo(FloatPoint(floorf(center.x()), ceilf(rect.y() + arrowEdge.height() + arrowEdge.height() + arrowEdge.height())));
    path.addLineTo(FloatPoint(ceilf(center.x() + arrowEdge.width()), center.y()));
    path.closeSubpath();
}

bool RenderEmbeddedObject::getReplacementTextGeometry(const LayoutPoint& accumulatedOffset, FloatRect& contentRect, Path& path, FloatRect& replacementTextRect, FloatRect& arrowRect, Font& font, TextRun& run, float& textWidth) const
{
    contentRect = contentBoxRect();
    contentRect.moveBy(roundedIntPoint(accumulatedOffset));

    FontDescription fontDescription;
    RenderTheme::defaultTheme()->systemFont(CSSValueWebkitSmallControl, fontDescription);
    fontDescription.setWeight(FontWeightBold);
    Settings* settings = document()->settings();
    ASSERT(settings);
    if (!settings)
        return false;
    fontDescription.setRenderingMode(settings->fontRenderingMode());
    fontDescription.setComputedSize(fontDescription.specifiedSize());
    font = Font(fontDescription, 0, 0);
    font.update(0);

    run = TextRun(m_unavailablePluginReplacementText);
    textWidth = font.width(run);

    replacementTextRect.setSize(FloatSize(textWidth + replacementTextRoundedRectLeftRightTextMargin * 2, replacementTextRoundedRectHeight));
    float x = (contentRect.size().width() / 2 - replacementTextRect.size().width() / 2) + contentRect.location().x();
    float y = (contentRect.size().height() / 2 - replacementTextRect.size().height() / 2) + contentRect.location().y();
    replacementTextRect.setLocation(FloatPoint(x, y));

    replacementTextRect.setHeight(replacementTextRect.height() + replacementTextRoundedRectBottomTextPadding);

    path.addRoundedRect(replacementTextRect, FloatSize(replacementTextRoundedRectRadius, replacementTextRoundedRectRadius));

    if (shouldUnavailablePluginMessageBeButton(document(), m_pluginUnavailabilityReason)) {
        arrowRect = path.boundingRect();
        arrowRect.setX(ceilf(arrowRect.maxX() + replacementArrowLeftMargin));
        arrowRect.setWidth(arrowRect.height());
        arrowRect.inflate(-0.5);
        path.addEllipse(arrowRect);
        addReplacementArrowPath(path, arrowRect);
    }

    return true;
}

LayoutRect RenderEmbeddedObject::unavailablePluginIndicatorBounds(const LayoutPoint& accumulatedOffset) const
{
    FloatRect contentRect;
    Path path;
    FloatRect replacementTextRect;
    FloatRect arrowRect;
    Font font;
    TextRun run("", 0);
    float textWidth;
    if (getReplacementTextGeometry(accumulatedOffset, contentRect, path, replacementTextRect, arrowRect, font, run, textWidth))
        return LayoutRect(path.boundingRect());

    return LayoutRect();
}

bool RenderEmbeddedObject::isReplacementObscured() const
{
    // Return whether or not the replacement content for blocked plugins is accessible to the user.

    // Check the opacity of each layer containing the element or its ancestors.
    float opacity = 1.0;
    for (RenderLayer* layer = enclosingLayer(); layer; layer = layer->parent()) {
        RenderLayerModelObject* renderer = layer->renderer();
        RenderStyle* style = renderer->style();
        opacity *= style->opacity();
        if (opacity < 0.1)
            return true;
    }

    // Calculate the absolute rect for the blocked plugin replacement text.
    IntRect absoluteBoundingBox = absoluteBoundingBoxRect();
    LayoutPoint absoluteLocation(absoluteBoundingBox.location());
    LayoutRect rect = unavailablePluginIndicatorBounds(absoluteLocation);
    if (rect.isEmpty())
        return true;

    RenderView* docRenderer = document()->renderView();
    ASSERT(docRenderer);
    if (!docRenderer)
        return true;
    
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);
    HitTestResult result;
    HitTestLocation location;
    
    LayoutUnit x = rect.x();
    LayoutUnit y = rect.y();
    LayoutUnit width = rect.width();
    LayoutUnit height = rect.height();
    
    // Hit test the center and near the corners of the replacement text to ensure
    // it is visible and is not masked by other elements.
    bool hit = false;
    location = LayoutPoint(x + width / 2, y + height / 2);
    hit = docRenderer->hitTest(request, location, result);
    if (!hit || result.innerNode() != node())
        return true;
    
    location = LayoutPoint(x, y);
    hit = docRenderer->hitTest(request, location, result);
    if (!hit || result.innerNode() != node())
        return true;
    
    location = LayoutPoint(x + width, y);
    hit = docRenderer->hitTest(request, location, result);
    if (!hit || result.innerNode() != node())
        return true;
    
    location = LayoutPoint(x + width, y + height);
    hit = docRenderer->hitTest(request, location, result);
    if (!hit || result.innerNode() != node())
        return true;
    
    location = LayoutPoint(x, y + height);
    hit = docRenderer->hitTest(request, location, result);
    if (!hit || result.innerNode() != node())
        return true;

    return false;
}

void RenderEmbeddedObject::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());

    LayoutSize oldSize = contentBoxRect().size();

    updateLogicalWidth();
    updateLogicalHeight();

    RenderPart::layout();

    m_overflow.clear();
    addVisualEffectOverflow();

    updateLayerTransform();

    bool wasMissingWidget = false;
    if (!widget() && frameView() && canHaveWidget()) {
        wasMissingWidget = true;
        frameView()->addWidgetToUpdate(this);
    }

    setNeedsLayout(false);

    LayoutSize newSize = contentBoxRect().size();

    if (!wasMissingWidget && newSize.width() >= oldSize.width() && newSize.height() >= oldSize.height()) {
        Element* element = toElement(node());
        if (element && element->isPluginElement() && toHTMLPlugInElement(element)->isPlugInImageElement()) {
            HTMLPlugInImageElement* plugInImageElement = toHTMLPlugInImageElement(element);
            if (plugInImageElement->displayState() > HTMLPlugInElement::DisplayingSnapshot && plugInImageElement->snapshotDecision() == HTMLPlugInImageElement::MaySnapshotWhenResized && document()->view()) {
                plugInImageElement->setNeedsCheckForSizeChange();
                document()->view()->addWidgetToUpdate(this);
            }
        }
    }

    if (!canHaveChildren())
        return;

    // This code copied from RenderMedia::layout().
    RenderObject* child = m_children.firstChild();

    if (!child)
        return;

    RenderBox* childBox = toRenderBox(child);

    if (!childBox)
        return;

    if (newSize == oldSize && !childBox->needsLayout())
        return;
    
    // When calling layout() on a child node, a parent must either push a LayoutStateMaintainter, or
    // instantiate LayoutStateDisabler. Since using a LayoutStateMaintainer is slightly more efficient,
    // and this method will be called many times per second during playback, use a LayoutStateMaintainer:
    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());
    
    childBox->setLocation(LayoutPoint(borderLeft(), borderTop()) + LayoutSize(paddingLeft(), paddingTop()));
    childBox->style()->setHeight(Length(newSize.height(), Fixed));
    childBox->style()->setWidth(Length(newSize.width(), Fixed));
    childBox->setNeedsLayout(true, MarkOnlyThis);
    childBox->layout();
    setChildNeedsLayout(false);
    
    statePusher.pop();
}

void RenderEmbeddedObject::viewCleared()
{
    // This is required for <object> elements whose contents are rendered by WebCore (e.g. src="foo.html").
    if (node() && widget() && widget()->isFrameView()) {
        FrameView* view = toFrameView(widget());
        int marginWidth = -1;
        int marginHeight = -1;
        if (node()->hasTagName(iframeTag)) {
            HTMLIFrameElement* frame = toHTMLIFrameElement(node());
            marginWidth = frame->marginWidth();
            marginHeight = frame->marginHeight();
        }
        if (marginWidth != -1)
            view->setMarginWidth(marginWidth);
        if (marginHeight != -1)
            view->setMarginHeight(marginHeight);
    }
}

bool RenderEmbeddedObject::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    if (!RenderPart::nodeAtPoint(request, result, locationInContainer, accumulatedOffset, hitTestAction))
        return false;

    if (!widget() || !widget()->isPluginViewBase())
        return true;

    PluginViewBase* view = toPluginViewBase(widget());
    IntPoint roundedPoint = locationInContainer.roundedPoint();

    if (Scrollbar* horizontalScrollbar = view->horizontalScrollbar()) {
        if (horizontalScrollbar->shouldParticipateInHitTesting() && horizontalScrollbar->frameRect().contains(roundedPoint)) {
            result.setScrollbar(horizontalScrollbar);
            return true;
        }
    }

    if (Scrollbar* verticalScrollbar = view->verticalScrollbar()) {
        if (verticalScrollbar->shouldParticipateInHitTesting() && verticalScrollbar->frameRect().contains(roundedPoint)) {
            result.setScrollbar(verticalScrollbar);
            return true;
        }
    }

    return true;
}

bool RenderEmbeddedObject::scroll(ScrollDirection direction, ScrollGranularity granularity, float, Node**)
{
    if (!widget() || !widget()->isPluginViewBase())
        return false;

    return toPluginViewBase(widget())->scroll(direction, granularity);
}

bool RenderEmbeddedObject::logicalScroll(ScrollLogicalDirection direction, ScrollGranularity granularity, float multiplier, Node** stopNode)
{
    // Plugins don't expose a writing direction, so assuming horizontal LTR.
    return scroll(logicalToPhysical(direction, true, false), granularity, multiplier, stopNode);
}


bool RenderEmbeddedObject::isInUnavailablePluginIndicator(const LayoutPoint& point) const
{
    FloatRect contentRect;
    Path path;
    FloatRect replacementTextRect;
    FloatRect arrowRect;
    Font font;
    TextRun run("");
    float textWidth;
    return getReplacementTextGeometry(IntPoint(), contentRect, path, replacementTextRect, arrowRect, font, run, textWidth)
        && (path.contains(point) || arrowRect.contains(point));
}

bool RenderEmbeddedObject::isInUnavailablePluginIndicator(MouseEvent* event) const
{
    return isInUnavailablePluginIndicator(roundedLayoutPoint(absoluteToLocal(event->absoluteLocation(), UseTransforms)));
}

void RenderEmbeddedObject::handleUnavailablePluginIndicatorEvent(Event* event)
{
    if (!shouldUnavailablePluginMessageBeButton(document(), m_pluginUnavailabilityReason))
        return;

    if (!event->isMouseEvent())
        return;

    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    HTMLPlugInElement* element = toHTMLPlugInElement(node());
    if (event->type() == eventNames().mousedownEvent && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        m_mouseDownWasInUnavailablePluginIndicator = isInUnavailablePluginIndicator(mouseEvent);
        if (m_mouseDownWasInUnavailablePluginIndicator) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(element);
                element->setIsCapturingMouseEvents(true);
            }
            setUnavailablePluginIndicatorIsPressed(true);
        }
        event->setDefaultHandled();
    }
    if (event->type() == eventNames().mouseupEvent && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        if (m_unavailablePluginIndicatorIsPressed) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(0);
                element->setIsCapturingMouseEvents(false);
            }
            setUnavailablePluginIndicatorIsPressed(false);
        }
        if (m_mouseDownWasInUnavailablePluginIndicator && isInUnavailablePluginIndicator(mouseEvent)) {
            if (Page* page = document()->page())
                page->chrome().client()->unavailablePluginButtonClicked(element, m_pluginUnavailabilityReason);
        }
        m_mouseDownWasInUnavailablePluginIndicator = false;
        event->setDefaultHandled();
    }
    if (event->type() == eventNames().mousemoveEvent) {
        setUnavailablePluginIndicatorIsPressed(m_mouseDownWasInUnavailablePluginIndicator && isInUnavailablePluginIndicator(mouseEvent));
        event->setDefaultHandled();
    }
}

CursorDirective RenderEmbeddedObject::getCursor(const LayoutPoint& point, Cursor& cursor) const
{
    if (showsUnavailablePluginIndicator() && shouldUnavailablePluginMessageBeButton(document(), m_pluginUnavailabilityReason) && isInUnavailablePluginIndicator(point)) {
        cursor = handCursor();
        return SetCursor;
    }
    return RenderPart::getCursor(point, cursor);
}

bool RenderEmbeddedObject::canHaveChildren() const
{
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    if (!node())
        return false;

    if (toElement(node())->isMediaElement())
        return true;
#endif

    if (isSnapshottedPlugIn())
        return true;

    return false;
}

}
