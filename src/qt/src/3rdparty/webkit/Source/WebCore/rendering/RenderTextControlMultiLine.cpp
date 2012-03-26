/**
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/) 
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
#include "RenderTextControlMultiLine.h"

#include "Event.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "HTMLTextAreaElement.h"
#include "HitTestResult.h"

namespace WebCore {

RenderTextControlMultiLine::RenderTextControlMultiLine(Node* node, bool placeholderVisible)
    : RenderTextControl(node, placeholderVisible)
{
}

RenderTextControlMultiLine::~RenderTextControlMultiLine()
{
    if (node() && node()->inDocument())
        static_cast<HTMLTextAreaElement*>(node())->rendererWillBeDestroyed();
}

void RenderTextControlMultiLine::subtreeHasChanged()
{
    RenderTextControl::subtreeHasChanged();
    HTMLTextAreaElement* textArea = static_cast<HTMLTextAreaElement*>(node());
    textArea->setChangedSinceLastFormControlChangeEvent(true);
    textArea->setFormControlValueMatchesRenderer(false);
    textArea->setNeedsValidityCheck();

    if (!node()->focused())
        return;

    if (Frame* frame = this->frame())
        frame->editor()->textDidChangeInTextArea(textArea);
}

bool RenderTextControlMultiLine::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, HitTestAction hitTestAction)
{
    if (!RenderTextControl::nodeAtPoint(request, result, x, y, tx, ty, hitTestAction))
        return false;

    if (result.innerNode() == node() || result.innerNode() == innerTextElement())
        hitInnerTextElement(result, x, y, tx, ty);

    return true;
}

void RenderTextControlMultiLine::forwardEvent(Event* event)
{
    RenderTextControl::forwardEvent(event);
}

float RenderTextControlMultiLine::getAvgCharWidth(AtomicString family)
{
    // Since Lucida Grande is the default font, we want this to match the width
    // of Courier New, the default font for textareas in IE, Firefox and Safari Win.
    // 1229 is the avgCharWidth value in the OS/2 table for Courier New.
    if (family == AtomicString("Lucida Grande"))
        return scaleEmToUnits(1229);

    return RenderTextControl::getAvgCharWidth(family);
}

int RenderTextControlMultiLine::preferredContentWidth(float charWidth) const
{
    int factor = static_cast<HTMLTextAreaElement*>(node())->cols();
    return static_cast<int>(ceilf(charWidth * factor)) + scrollbarThickness();
}

void RenderTextControlMultiLine::adjustControlHeightBasedOnLineHeight(int lineHeight)
{
    setHeight(height() + lineHeight * static_cast<HTMLTextAreaElement*>(node())->rows());
}

int RenderTextControlMultiLine::baselinePosition(FontBaseline baselineType, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    return RenderBox::baselinePosition(baselineType, firstLine, direction, linePositionMode);
}

void RenderTextControlMultiLine::updateFromElement()
{
    createSubtreeIfNeeded(0);
    RenderTextControl::updateFromElement();

    setInnerTextValue(static_cast<HTMLTextAreaElement*>(node())->value());
}

void RenderTextControlMultiLine::cacheSelection(int start, int end)
{
    static_cast<HTMLTextAreaElement*>(node())->cacheSelection(start, end);
}

PassRefPtr<RenderStyle> RenderTextControlMultiLine::createInnerTextStyle(const RenderStyle* startStyle) const
{
    RefPtr<RenderStyle> textBlockStyle = RenderStyle::create();
    textBlockStyle->inheritFrom(startStyle);
    adjustInnerTextStyle(startStyle, textBlockStyle.get());
    textBlockStyle->setDisplay(BLOCK);

    return textBlockStyle.release();
}

RenderStyle* RenderTextControlMultiLine::textBaseStyle() const
{
    return style();
}
    
int RenderTextControlMultiLine::textBlockInsetLeft() const
{
    int inset = borderLeft() + paddingLeft();
    if (HTMLElement* innerText = innerTextElement()) {
        if (RenderBox* innerTextRenderer = innerText->renderBox())
            inset += innerTextRenderer->paddingLeft();
    }
    return inset;
}

int RenderTextControlMultiLine::textBlockInsetRight() const
{
    int inset = borderRight() + paddingRight();
    if (HTMLElement* innerText = innerTextElement()) {
        if (RenderBox* innerTextRenderer = innerText->renderBox())
            inset += innerTextRenderer->paddingRight();
    }
    return inset;
}

int RenderTextControlMultiLine::textBlockInsetTop() const
{
    int inset = borderTop() + paddingTop();
    if (HTMLElement* innerText = innerTextElement()) {
        if (RenderBox* innerTextRenderer = innerText->renderBox())
            inset += innerTextRenderer->paddingTop();
    }
    return inset;
}
    
}
