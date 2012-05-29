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
#include "BaseCheckableInputType.h"

#include "FormDataList.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "KeyboardEvent.h"
#include "RegularExpression.h"

namespace WebCore {

bool BaseCheckableInputType::saveFormControlState(String& result) const
{
    result = element()->checked() ? "on" : "off";
    return true;
}

void BaseCheckableInputType::restoreFormControlState(const String& state) const
{
    element()->setChecked(state == "on");
}

bool BaseCheckableInputType::appendFormData(FormDataList& encoding, bool) const
{
    if (!element()->checked())
        return false;
    encoding.appendData(element()->name(), element()->value());
    return true;
}

void BaseCheckableInputType::handleKeydownEvent(KeyboardEvent* event)
{
    const String& key = event->keyIdentifier();
    if (key == "U+0020") {
        element()->setActive(true, true);
        // No setDefaultHandled(), because IE dispatches a keypress in this case
        // and the caller will only dispatch a keypress if we don't call setDefaultHandled().
    }
}

void BaseCheckableInputType::handleKeypressEvent(KeyboardEvent* event)
{
    if (event->charCode() == ' ') {
        // Prevent scrolling down the page.
        event->setDefaultHandled();
    }
}

bool BaseCheckableInputType::canSetStringValue() const
{
    return false;
}

// FIXME: Could share this with BaseButtonInputType and RangeInputType if we had a common base class.
void BaseCheckableInputType::accessKeyAction(bool sendToAnyElement)
{
    InputType::accessKeyAction(sendToAnyElement);

    // Send mouse button events if the caller specified sendToAnyElement.
    // FIXME: The comment above is no good. It says what we do, but not why.
    element()->dispatchSimulatedClick(0, sendToAnyElement);
}

String BaseCheckableInputType::fallbackValue()
{
    return element()->checked() ? "on" : "";
}

bool BaseCheckableInputType::storesValueSeparateFromAttribute()
{
    return false;
}

bool BaseCheckableInputType::isCheckable()
{
    return true;
}

} // namespace WebCore
