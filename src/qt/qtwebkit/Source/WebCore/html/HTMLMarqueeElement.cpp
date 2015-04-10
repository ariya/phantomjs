/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2007, 2010 Apple Inc. All rights reserved.
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
#include "HTMLMarqueeElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "RenderLayer.h"
#include "RenderMarquee.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLMarqueeElement::HTMLMarqueeElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , ActiveDOMObject(document)
{
    ASSERT(hasTagName(marqueeTag));
}

PassRefPtr<HTMLMarqueeElement> HTMLMarqueeElement::create(const QualifiedName& tagName, Document* document)
{
    RefPtr<HTMLMarqueeElement> marqueeElement(adoptRef(new HTMLMarqueeElement(tagName, document)));
    marqueeElement->suspendIfNeeded();
    return marqueeElement.release();
}

int HTMLMarqueeElement::minimumDelay() const
{
    if (fastGetAttribute(truespeedAttr).isEmpty()) {
        // WinIE uses 60ms as the minimum delay by default.
        return 60;
    }
    return 0;
}

bool HTMLMarqueeElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == widthAttr || name == heightAttr || name == bgcolorAttr || name == vspaceAttr || name == hspaceAttr || name == scrollamountAttr || name == scrolldelayAttr || name == loopAttr || name == behaviorAttr || name == directionAttr)
        return true;
    return HTMLElement::isPresentationAttribute(name);
}

void HTMLMarqueeElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == widthAttr) {
        if (!value.isEmpty())
            addHTMLLengthToStyle(style, CSSPropertyWidth, value);
    } else if (name == heightAttr) {
        if (!value.isEmpty())
            addHTMLLengthToStyle(style, CSSPropertyHeight, value);
    } else if (name == bgcolorAttr) {
        if (!value.isEmpty())
            addHTMLColorToStyle(style, CSSPropertyBackgroundColor, value);
    } else if (name == vspaceAttr) {
        if (!value.isEmpty()) {
            addHTMLLengthToStyle(style, CSSPropertyMarginTop, value);
            addHTMLLengthToStyle(style, CSSPropertyMarginBottom, value);
        }
    } else if (name == hspaceAttr) {
        if (!value.isEmpty()) {
            addHTMLLengthToStyle(style, CSSPropertyMarginLeft, value);
            addHTMLLengthToStyle(style, CSSPropertyMarginRight, value);
        }
    } else if (name == scrollamountAttr) {
        if (!value.isEmpty())
            addHTMLLengthToStyle(style, CSSPropertyWebkitMarqueeIncrement, value);
    } else if (name == scrolldelayAttr) {
        if (!value.isEmpty())
            addHTMLLengthToStyle(style, CSSPropertyWebkitMarqueeSpeed, value);
    } else if (name == loopAttr) {
        if (!value.isEmpty()) {
            if (value == "-1" || equalIgnoringCase(value, "infinite"))
                addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitMarqueeRepetition, CSSValueInfinite);
            else
                addHTMLLengthToStyle(style, CSSPropertyWebkitMarqueeRepetition, value);
        }
    } else if (name == behaviorAttr) {
        if (!value.isEmpty())
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitMarqueeStyle, value);
    } else if (name == directionAttr) {
        if (!value.isEmpty())
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitMarqueeDirection, value);
    } else
        HTMLElement::collectStyleForPresentationAttribute(name, value, style);
}

void HTMLMarqueeElement::start()
{
    if (RenderMarquee* marqueeRenderer = renderMarquee())
        marqueeRenderer->start();
}

void HTMLMarqueeElement::stop()
{
    if (RenderMarquee* marqueeRenderer = renderMarquee())
        marqueeRenderer->stop();
}

int HTMLMarqueeElement::scrollAmount() const
{
    bool ok;
    int scrollAmount = fastGetAttribute(scrollamountAttr).toInt(&ok);
    return ok && scrollAmount >= 0 ? scrollAmount : RenderStyle::initialMarqueeIncrement().intValue();
}
    
void HTMLMarqueeElement::setScrollAmount(int scrollAmount, ExceptionCode& ec)
{
    if (scrollAmount < 0)
        ec = INDEX_SIZE_ERR;
    else
        setIntegralAttribute(scrollamountAttr, scrollAmount);
}
    
int HTMLMarqueeElement::scrollDelay() const
{
    bool ok;
    int scrollDelay = fastGetAttribute(scrolldelayAttr).toInt(&ok);
    return ok && scrollDelay >= 0 ? scrollDelay : RenderStyle::initialMarqueeSpeed();
}

void HTMLMarqueeElement::setScrollDelay(int scrollDelay, ExceptionCode& ec)
{
    if (scrollDelay < 0)
        ec = INDEX_SIZE_ERR;
    else
        setIntegralAttribute(scrolldelayAttr, scrollDelay);
}
    
int HTMLMarqueeElement::loop() const
{
    bool ok;
    int loopValue = fastGetAttribute(loopAttr).toInt(&ok);
    return ok && loopValue > 0 ? loopValue : -1;
}
    
void HTMLMarqueeElement::setLoop(int loop, ExceptionCode& ec)
{
    if (loop <= 0 && loop != -1)
        ec = INDEX_SIZE_ERR;
    else
        setIntegralAttribute(loopAttr, loop);
}

bool HTMLMarqueeElement::canSuspend() const
{
    return true;
}

void HTMLMarqueeElement::suspend(ReasonForSuspension)
{
    if (RenderMarquee* marqueeRenderer = renderMarquee())
        marqueeRenderer->suspend();
}

void HTMLMarqueeElement::resume()
{
    if (RenderMarquee* marqueeRenderer = renderMarquee())
        marqueeRenderer->updateMarqueePosition();
}

RenderMarquee* HTMLMarqueeElement::renderMarquee() const
{
    if (renderer() && renderer()->hasLayer())
        return renderBoxModelObject()->layer()->marquee();
    return 0;
}

} // namespace WebCore
