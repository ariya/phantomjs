/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
#include "BaseChooserOnlyDateAndTimeInputType.h"

#include "Chrome.h"
#include "HTMLDivElement.h"
#include "HTMLInputElement.h"
#include "Page.h"
#include "ScriptController.h"
#include "ShadowRoot.h"

namespace WebCore {

BaseChooserOnlyDateAndTimeInputType::~BaseChooserOnlyDateAndTimeInputType()
{
    closeDateTimeChooser();
}

void BaseChooserOnlyDateAndTimeInputType::handleDOMActivateEvent(Event*)
{
    if (element()->isDisabledOrReadOnly() || !element()->renderer() || !ScriptController::processingUserGesture())
        return;

    if (m_dateTimeChooser)
        return;
    if (!element()->document()->page())
        return;
    DateTimeChooserParameters parameters;
    if (!element()->setupDateTimeChooserParameters(parameters))
        return;
    m_dateTimeChooser = element()->document()->page()->chrome().openDateTimeChooser(this, parameters);
}

void BaseChooserOnlyDateAndTimeInputType::createShadowSubtree()
{
    DEFINE_STATIC_LOCAL(AtomicString, valueContainerPseudo, ("-webkit-date-and-time-value", AtomicString::ConstructFromLiteral));

    RefPtr<HTMLDivElement> valueContainer = HTMLDivElement::create(element()->document());
    valueContainer->setPseudo(valueContainerPseudo);
    element()->userAgentShadowRoot()->appendChild(valueContainer.get());
    updateAppearance();
}

void BaseChooserOnlyDateAndTimeInputType::updateAppearance()
{
    Node* node = element()->userAgentShadowRoot()->firstChild();
    if (!node || !node->isHTMLElement())
        return;
    String displayValue = visibleValue();
    if (displayValue.isEmpty()) {
        // Need to put something to keep text baseline.
        displayValue = ASCIILiteral(" ");
    }
    toHTMLElement(node)->setInnerText(displayValue, ASSERT_NO_EXCEPTION);
}

void BaseChooserOnlyDateAndTimeInputType::setValue(const String& value, bool valueChanged, TextFieldEventBehavior eventBehavior)
{
    BaseDateAndTimeInputType::setValue(value, valueChanged, eventBehavior);
    if (valueChanged)
        updateAppearance();
}

void BaseChooserOnlyDateAndTimeInputType::detach()
{
    closeDateTimeChooser();
}

void BaseChooserOnlyDateAndTimeInputType::didChooseValue(const String& value)
{
    element()->setValue(value, DispatchInputAndChangeEvent);
}

void BaseChooserOnlyDateAndTimeInputType::didEndChooser()
{
    m_dateTimeChooser.clear();
}

void BaseChooserOnlyDateAndTimeInputType::closeDateTimeChooser()
{
    if (m_dateTimeChooser)
        m_dateTimeChooser->endChooser();
}

void BaseChooserOnlyDateAndTimeInputType::handleKeydownEvent(KeyboardEvent* event)
{
    BaseClickableWithKeyInputType::handleKeydownEvent(element(), event);
}

void BaseChooserOnlyDateAndTimeInputType::handleKeypressEvent(KeyboardEvent* event)
{
    BaseClickableWithKeyInputType::handleKeypressEvent(element(), event);
}

void BaseChooserOnlyDateAndTimeInputType::handleKeyupEvent(KeyboardEvent* event)
{
    BaseClickableWithKeyInputType::handleKeyupEvent(*this, event);
}

void BaseChooserOnlyDateAndTimeInputType::accessKeyAction(bool sendMouseEvents)
{
    BaseDateAndTimeInputType::accessKeyAction(sendMouseEvents);
    BaseClickableWithKeyInputType::accessKeyAction(element(), sendMouseEvents);
}

bool BaseChooserOnlyDateAndTimeInputType::isMouseFocusable() const
{
    return element()->isTextFormControlFocusable();
}

}
#endif
