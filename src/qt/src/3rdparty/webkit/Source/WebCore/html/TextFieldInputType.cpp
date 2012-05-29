/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextFieldInputType.h"

#include "Frame.h"
#include "HTMLInputElement.h"
#include "KeyboardEvent.h"
#include "RenderTextControlSingleLine.h"
#include "TextEvent.h"
#include "WheelEvent.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

bool TextFieldInputType::isTextField() const
{
    return true;
}

bool TextFieldInputType::valueMissing(const String& value) const
{
    return value.isEmpty();
}

bool TextFieldInputType::canSetSuggestedValue()
{
    return true;
}

void TextFieldInputType::handleKeydownEvent(KeyboardEvent* event)
{
    if (!element()->focused())
        return;
    Frame* frame = element()->document()->frame();
    if (!frame || !frame->editor()->doTextFieldCommandFromEvent(element(), event))
        return;
    event->setDefaultHandled();
}

void TextFieldInputType::handleKeydownEventForSpinButton(KeyboardEvent* event)
{
    if (element()->disabled() || element()->readOnly())
        return;
    const String& key = event->keyIdentifier();
    int step = 0;
    if (key == "Up")
        step = 1;
    else if (key == "Down")
        step = -1;
    else
        return;
    element()->stepUpFromRenderer(step);
    event->setDefaultHandled();
}

void TextFieldInputType::handleWheelEventForSpinButton(WheelEvent* event)
{
    if (element()->disabled() || element()->readOnly() || !element()->focused())
        return;
    int step = 0;
    if (event->wheelDeltaY() > 0)
        step = 1;
    else if (event->wheelDeltaY() < 0)
        step = -1;
    else
        return;
    element()->stepUpFromRenderer(step);
    event->setDefaultHandled();
}

void TextFieldInputType::forwardEvent(Event* event)
{
    if (element()->renderer() && (event->isMouseEvent() || event->isDragEvent() || event->isWheelEvent() || event->type() == eventNames().blurEvent || event->type() == eventNames().focusEvent))
        toRenderTextControlSingleLine(element()->renderer())->forwardEvent(event);
}

bool TextFieldInputType::shouldSubmitImplicitly(Event* event)
{
    return (event->type() == eventNames().textInputEvent && event->isTextEvent() && static_cast<TextEvent*>(event)->data() == "\n") || InputType::shouldSubmitImplicitly(event);
}

RenderObject* TextFieldInputType::createRenderer(RenderArena* arena, RenderStyle*) const
{
    return new (arena) RenderTextControlSingleLine(element(), element()->placeholderShouldBeVisible());
}

bool TextFieldInputType::shouldUseInputMethod() const
{
    return true;
}

String TextFieldInputType::sanitizeValue(const String& proposedValue)
{
    return InputElement::sanitizeValueForTextField(element(), proposedValue);
}

bool TextFieldInputType::shouldRespectListAttribute()
{
    return true;
}

} // namespace WebCore
