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

#include "Chrome.h"
#include "ChromeClient.h"
#include "CSSValueKeywords.h"
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
#include "LocalizedStrings.h"
#include "MIMETypeRegistry.h"
#include "MouseEvent.h"
#include "Page.h"
#include "PaintInfo.h"
#include "Path.h"
#include "PluginViewBase.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidgetProtector.h"
#include "Settings.h"
#include "Text.h"
#include "TextRun.h"

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#include "HTMLVideoElement.h"
#endif

namespace WebCore {

using namespace HTMLNames;
    
static const float replacementTextRoundedRectHeight = 18;
static const float replacementTextRoundedRectLeftRightTextMargin = 6;
static const float replacementTextRoundedRectOpacity = 0.20f;
static const float replacementTextPressedRoundedRectOpacity = 0.65f;
static const float replacementTextRoundedRectRadius = 5;
static const float replacementTextTextOpacity = 0.55f;
static const float replacementTextPressedTextOpacity = 0.65f;

static const Color& replacementTextRoundedRectPressedColor()
{
    static const Color lightGray(205, 205, 205);
    return lightGray;
}
    
RenderEmbeddedObject::RenderEmbeddedObject(Element* element)
    : RenderPart(element)
    , m_hasFallbackContent(false)
    , m_showsMissingPluginIndicator(false)
    , m_missingPluginIndicatorIsPressed(false)
    , m_mouseDownWasInMissingPluginIndicator(false)
{
    view()->frameView()->setIsVisuallyNonEmpty();
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    if (element->hasTagName(videoTag) || element->hasTagName(audioTag))
        setHasIntrinsicSize();
#endif
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
    return widget() && widget()->isPluginViewBase() && static_cast<PluginViewBase*>(widget())->platformLayer();
}
#endif

void RenderEmbeddedObject::setShowsMissingPluginIndicator()
{
    ASSERT(m_replacementText.isEmpty());
    m_replacementText = missingPluginText();
    m_showsMissingPluginIndicator = true;
}

void RenderEmbeddedObject::setShowsCrashedPluginIndicator()
{
    ASSERT(m_replacementText.isEmpty());
    m_replacementText = crashedPluginText();
}

bool RenderEmbeddedObject::pluginCrashedOrWasMissing() const
{
    return !m_replacementText.isNull();
}
    
void RenderEmbeddedObject::setMissingPluginIndicatorIsPressed(bool pressed)
{
    if (m_missingPluginIndicatorIsPressed == pressed)
        return;
    
    m_missingPluginIndicatorIsPressed = pressed;
    repaint();
}

void RenderEmbeddedObject::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (pluginCrashedOrWasMissing()) {
        RenderReplaced::paint(paintInfo, tx, ty);
        return;
    }
    
    RenderPart::paint(paintInfo, tx, ty);
}

void RenderEmbeddedObject::paintReplaced(PaintInfo& paintInfo, int tx, int ty)
{
    if (!pluginCrashedOrWasMissing())
        return;

    if (paintInfo.phase == PaintPhaseSelection)
        return;
    
    GraphicsContext* context = paintInfo.context;
    if (context->paintingDisabled())
        return;
    
    FloatRect contentRect;
    Path path;
    FloatRect replacementTextRect;
    Font font;
    TextRun run("");
    float textWidth;
    if (!getReplacementTextGeometry(tx, ty, contentRect, path, replacementTextRect, font, run, textWidth))
        return;
    
    GraphicsContextStateSaver stateSaver(*context);
    context->clip(contentRect);
    context->setAlpha(m_missingPluginIndicatorIsPressed ? replacementTextPressedRoundedRectOpacity : replacementTextRoundedRectOpacity);
    context->setFillColor(m_missingPluginIndicatorIsPressed ? replacementTextRoundedRectPressedColor() : Color::white, style()->colorSpace());
    context->fillPath(path);

    const FontMetrics& fontMetrics = font.fontMetrics();
    float labelX = roundf(replacementTextRect.location().x() + (replacementTextRect.size().width() - textWidth) / 2);
    float labelY = roundf(replacementTextRect.location().y() + (replacementTextRect.size().height() - fontMetrics.height()) / 2 + fontMetrics.ascent());
    context->setAlpha(m_missingPluginIndicatorIsPressed ? replacementTextPressedTextOpacity : replacementTextTextOpacity);
    context->setFillColor(Color::black, style()->colorSpace());
    context->drawBidiText(font, run, FloatPoint(labelX, labelY));
}

bool RenderEmbeddedObject::getReplacementTextGeometry(int tx, int ty, FloatRect& contentRect, Path& path, FloatRect& replacementTextRect, Font& font, TextRun& run, float& textWidth)
{
    contentRect = contentBoxRect();
    contentRect.move(tx, ty);
    
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
    
    run = TextRun(m_replacementText.characters(), m_replacementText.length());
    textWidth = font.width(run);
    
    replacementTextRect.setSize(FloatSize(textWidth + replacementTextRoundedRectLeftRightTextMargin * 2, replacementTextRoundedRectHeight));
    float x = (contentRect.size().width() / 2 - replacementTextRect.size().width() / 2) + contentRect.location().x();
    float y = (contentRect.size().height() / 2 - replacementTextRect.size().height() / 2) + contentRect.location().y();
    replacementTextRect.setLocation(FloatPoint(x, y));
    
    path.addRoundedRect(replacementTextRect, FloatSize(replacementTextRoundedRectRadius, replacementTextRoundedRectRadius));

    return true;
}

void RenderEmbeddedObject::layout()
{
    ASSERT(needsLayout());

    computeLogicalWidth();
    computeLogicalHeight();

    RenderPart::layout();

    m_overflow.clear();
    addShadowOverflow();

    updateLayerTransform();

    if (!widget() && frameView())
        frameView()->addWidgetToUpdate(this);

    setNeedsLayout(false);
}

void RenderEmbeddedObject::viewCleared()
{
    // This is required for <object> elements whose contents are rendered by WebCore (e.g. src="foo.html").
    if (node() && widget() && widget()->isFrameView()) {
        FrameView* view = static_cast<FrameView*>(widget());
        int marginWidth = -1;
        int marginHeight = -1;
        if (node()->hasTagName(iframeTag)) {
            HTMLIFrameElement* frame = static_cast<HTMLIFrameElement*>(node());
            marginWidth = frame->marginWidth();
            marginHeight = frame->marginHeight();
        }
        if (marginWidth != -1)
            view->setMarginWidth(marginWidth);
        if (marginHeight != -1)
            view->setMarginHeight(marginHeight);
    }
}
 
bool RenderEmbeddedObject::isInMissingPluginIndicator(MouseEvent* event)
{
    FloatRect contentRect;
    Path path;
    FloatRect replacementTextRect;
    Font font;
    TextRun run("");
    float textWidth;
    if (!getReplacementTextGeometry(0, 0, contentRect, path, replacementTextRect, font, run, textWidth))
        return false;
    
    return path.contains(absoluteToLocal(event->absoluteLocation(), false, true));
}

void RenderEmbeddedObject::handleMissingPluginIndicatorEvent(Event* event)
{
    if (Page* page = document()->page()) {
        if (!page->chrome()->client()->shouldMissingPluginMessageBeButton())
            return;
    }
    
    if (!event->isMouseEvent())
        return;
    
    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    HTMLPlugInElement* element = static_cast<HTMLPlugInElement*>(node());
    if (event->type() == eventNames().mousedownEvent && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        m_mouseDownWasInMissingPluginIndicator = isInMissingPluginIndicator(mouseEvent);
        if (m_mouseDownWasInMissingPluginIndicator) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(element);
                element->setIsCapturingMouseEvents(true);
            }
            setMissingPluginIndicatorIsPressed(true);
        }
        event->setDefaultHandled();
    }        
    if (event->type() == eventNames().mouseupEvent && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        if (m_missingPluginIndicatorIsPressed) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(0);
                element->setIsCapturingMouseEvents(false);
            }
            setMissingPluginIndicatorIsPressed(false);
        }
        if (m_mouseDownWasInMissingPluginIndicator && isInMissingPluginIndicator(mouseEvent)) {
            if (Page* page = document()->page())
                page->chrome()->client()->missingPluginButtonClicked(element);            
        }
        m_mouseDownWasInMissingPluginIndicator = false;
        event->setDefaultHandled();
    }
    if (event->type() == eventNames().mousemoveEvent) {
        setMissingPluginIndicatorIsPressed(m_mouseDownWasInMissingPluginIndicator && isInMissingPluginIndicator(mouseEvent));
        event->setDefaultHandled();
    }
}

}
