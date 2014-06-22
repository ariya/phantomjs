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

#include "BeforeTextInsertedEvent.h"
#include "Chrome.h"
#include "Editor.h"
#include "ElementShadow.h"
#include "FormDataList.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "KeyboardEvent.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "RenderLayer.h"
#include "RenderTextControlSingleLine.h"
#include "RenderTheme.h"
#include "ShadowRoot.h"
#include "TextControlInnerElements.h"
#include "TextEvent.h"
#include "TextIterator.h"
#include "WheelEvent.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

using namespace HTMLNames;

TextFieldInputType::TextFieldInputType(HTMLInputElement* element)
    : InputType(element)
{
}

TextFieldInputType::~TextFieldInputType()
{
    if (m_innerSpinButton)
        m_innerSpinButton->removeSpinButtonOwner();
}

bool TextFieldInputType::isKeyboardFocusable(KeyboardEvent*) const
{
    return element()->isTextFormControlFocusable();
}

bool TextFieldInputType::isMouseFocusable() const
{
    return element()->isTextFormControlFocusable();
}

bool TextFieldInputType::isTextField() const
{
    return true;
}

bool TextFieldInputType::valueMissing(const String& value) const
{
    return element()->isRequired() && value.isEmpty();
}

bool TextFieldInputType::canSetSuggestedValue()
{
    return true;
}

void TextFieldInputType::setValue(const String& sanitizedValue, bool valueChanged, TextFieldEventBehavior eventBehavior)
{
    // Grab this input element to keep reference even if JS event handler
    // changes input type.
    RefPtr<HTMLInputElement> input(element());

    // We don't ask InputType::setValue to dispatch events because
    // TextFieldInputType dispatches events different way from InputType.
    InputType::setValue(sanitizedValue, valueChanged, DispatchNoEvent);

    if (valueChanged)
        updateInnerTextValue();

    unsigned max = visibleValue().length();
    if (input->focused())
        input->setSelectionRange(max, max);
    else
        input->cacheSelectionInResponseToSetValue(max);

    if (!valueChanged)
        return;

    switch (eventBehavior) {
    case DispatchChangeEvent:
        // If the user is still editing this field, dispatch an input event rather than a change event.
        // The change event will be dispatched when editing finishes.
        if (input->focused())
            input->dispatchFormControlInputEvent();
        else
            input->dispatchFormControlChangeEvent();
        break;

    case DispatchInputAndChangeEvent: {
        input->dispatchFormControlInputEvent();
        input->dispatchFormControlChangeEvent();
        break;
    }

    case DispatchNoEvent:
        break;
    }

    // FIXME: Why do we do this when eventBehavior == DispatchNoEvent
    if (!input->focused() || eventBehavior == DispatchNoEvent)
        input->setTextAsOfLastFormControlChangeEvent(sanitizedValue);
}

void TextFieldInputType::handleKeydownEvent(KeyboardEvent* event)
{
    if (!element()->focused())
        return;
    Frame* frame = element()->document()->frame();
    if (!frame || !frame->editor().doTextFieldCommandFromEvent(element(), event))
        return;
    event->setDefaultHandled();
}

void TextFieldInputType::handleKeydownEventForSpinButton(KeyboardEvent* event)
{
    if (element()->isDisabledOrReadOnly())
        return;
    const String& key = event->keyIdentifier();
    if (key == "Up")
        spinButtonStepUp();
    else if (key == "Down")
        spinButtonStepDown();
    else
        return;
    event->setDefaultHandled();
}

void TextFieldInputType::forwardEvent(Event* event)
{
    if (m_innerSpinButton) {
        m_innerSpinButton->forwardEvent(event);
        if (event->defaultHandled())
            return;
    }

    if (element()->renderer() && (event->isMouseEvent() || event->isDragEvent() || event->hasInterface(eventNames().interfaceForWheelEvent) || event->type() == eventNames().blurEvent || event->type() == eventNames().focusEvent)) {
        RenderTextControlSingleLine* renderTextControl = toRenderTextControlSingleLine(element()->renderer());
        if (event->type() == eventNames().blurEvent) {
            if (RenderBox* innerTextRenderer = innerTextElement()->renderBox()) {
                if (RenderLayer* innerLayer = innerTextRenderer->layer()) {
                    IntSize scrollOffset(!renderTextControl->style()->isLeftToRightDirection() ? innerLayer->scrollWidth() : 0, 0);
                    innerLayer->scrollToOffset(scrollOffset, RenderLayer::ScrollOffsetClamped);
                }
            }

            renderTextControl->capsLockStateMayHaveChanged();
        } else if (event->type() == eventNames().focusEvent)
            renderTextControl->capsLockStateMayHaveChanged();

        element()->forwardEvent(event);
    }
}

void TextFieldInputType::handleBlurEvent()
{
    InputType::handleBlurEvent();
    element()->endEditing();
}

bool TextFieldInputType::shouldSubmitImplicitly(Event* event)
{
    return (event->type() == eventNames().textInputEvent && event->hasInterface(eventNames().interfaceForTextEvent) && static_cast<TextEvent*>(event)->data() == "\n") || InputType::shouldSubmitImplicitly(event);
}

RenderObject* TextFieldInputType::createRenderer(RenderArena* arena, RenderStyle*) const
{
    return new (arena) RenderTextControlSingleLine(element());
}

bool TextFieldInputType::needsContainer() const
{
#if ENABLE(INPUT_SPEECH)
    return element()->isSpeechEnabled();
#else
    return false;
#endif
}

bool TextFieldInputType::shouldHaveSpinButton() const
{
    Document* document = element()->document();
    RefPtr<RenderTheme> theme = document->page() ? document->page()->theme() : RenderTheme::defaultTheme();
    return theme->shouldHaveSpinButton(element());
}

void TextFieldInputType::createShadowSubtree()
{
    ASSERT(element()->shadow());

    ASSERT(!m_innerText);
    ASSERT(!m_innerBlock);
    ASSERT(!m_innerSpinButton);

    Document* document = element()->document();
    bool shouldHaveSpinButton = this->shouldHaveSpinButton();
    bool createsContainer = shouldHaveSpinButton || needsContainer();

    m_innerText = TextControlInnerTextElement::create(document);
    if (!createsContainer) {
        element()->userAgentShadowRoot()->appendChild(m_innerText, IGNORE_EXCEPTION);
        return;
    }

    ShadowRoot* shadowRoot = element()->userAgentShadowRoot();
    m_container = TextControlInnerContainer::create(document);
    m_container->setPseudo(AtomicString("-webkit-textfield-decoration-container", AtomicString::ConstructFromLiteral));
    shadowRoot->appendChild(m_container, IGNORE_EXCEPTION);

    m_innerBlock = TextControlInnerElement::create(document);
    m_innerBlock->appendChild(m_innerText, IGNORE_EXCEPTION);
    m_container->appendChild(m_innerBlock, IGNORE_EXCEPTION);

#if ENABLE(INPUT_SPEECH)
    ASSERT(!m_speechButton);
    if (element()->isSpeechEnabled()) {
        m_speechButton = InputFieldSpeechButtonElement::create(document);
        m_container->appendChild(m_speechButton, IGNORE_EXCEPTION);
    }
#endif

    if (shouldHaveSpinButton) {
        m_innerSpinButton = SpinButtonElement::create(document, *this);
        m_container->appendChild(m_innerSpinButton, IGNORE_EXCEPTION);
    }
}

HTMLElement* TextFieldInputType::containerElement() const
{
    return m_container.get();
}

HTMLElement* TextFieldInputType::innerBlockElement() const
{
    return m_innerBlock.get();
}

HTMLElement* TextFieldInputType::innerTextElement() const
{
    ASSERT(m_innerText);
    return m_innerText.get();
}

HTMLElement* TextFieldInputType::innerSpinButtonElement() const
{
    return m_innerSpinButton.get();
}

#if ENABLE(INPUT_SPEECH)
HTMLElement* TextFieldInputType::speechButtonElement() const
{
    return m_speechButton.get();
}
#endif

HTMLElement* TextFieldInputType::placeholderElement() const
{
    return m_placeholder.get();
}

void TextFieldInputType::destroyShadowSubtree()
{
    InputType::destroyShadowSubtree();
    m_innerText.clear();
    m_placeholder.clear();
    m_innerBlock.clear();
#if ENABLE(INPUT_SPEECH)
    m_speechButton.clear();
#endif
    if (m_innerSpinButton)
        m_innerSpinButton->removeSpinButtonOwner();
    m_innerSpinButton.clear();
    m_container.clear();
}

void TextFieldInputType::attributeChanged()
{
    // FIXME: Updating the inner text on any attribute update should
    // be unnecessary. We should figure out what attributes affect.
    updateInnerTextValue();
}

void TextFieldInputType::disabledAttributeChanged()
{
    if (m_innerSpinButton)
        m_innerSpinButton->releaseCapture();
}

void TextFieldInputType::readonlyAttributeChanged()
{
    if (m_innerSpinButton)
        m_innerSpinButton->releaseCapture();
}

bool TextFieldInputType::supportsReadOnly() const
{
    return true;
}

bool TextFieldInputType::shouldUseInputMethod() const
{
    return true;
}

static bool isASCIILineBreak(UChar c)
{
    return c == '\r' || c == '\n';
}

static String limitLength(const String& string, int maxLength)
{
    unsigned newLength = numCharactersInGraphemeClusters(string, maxLength);
    for (unsigned i = 0; i < newLength; ++i) {
        const UChar current = string[i];
        if (current < ' ' && current != '\t') {
            newLength = i;
            break;
        }
    }
    return string.left(newLength);
}

String TextFieldInputType::sanitizeValue(const String& proposedValue) const
{
    return limitLength(proposedValue.removeCharacters(isASCIILineBreak), HTMLInputElement::maximumLength);
}

void TextFieldInputType::handleBeforeTextInsertedEvent(BeforeTextInsertedEvent* event)
{
    // Make sure that the text to be inserted will not violate the maxLength.

    // We use RenderTextControlSingleLine::text() instead of InputElement::value()
    // because they can be mismatched by sanitizeValue() in
    // HTMLInputElement::subtreeHasChanged() in some cases.
    unsigned oldLength = numGraphemeClusters(element()->innerTextValue());

    // selectionLength represents the selection length of this text field to be
    // removed by this insertion.
    // If the text field has no focus, we don't need to take account of the
    // selection length. The selection is the source of text drag-and-drop in
    // that case, and nothing in the text field will be removed.
    unsigned selectionLength = element()->focused() ? numGraphemeClusters(plainText(element()->document()->frame()->selection()->selection().toNormalizedRange().get())) : 0;
    ASSERT(oldLength >= selectionLength);

    // Selected characters will be removed by the next text event.
    unsigned baseLength = oldLength - selectionLength;
    unsigned maxLength = static_cast<unsigned>(isTextType() ? element()->maxLength() : HTMLInputElement::maximumLength); // maxLength can never be negative.
    unsigned appendableLength = maxLength > baseLength ? maxLength - baseLength : 0;

    // Truncate the inserted text to avoid violating the maxLength and other constraints.
    String eventText = event->text();
    unsigned textLength = eventText.length();
    while (textLength > 0 && isASCIILineBreak(eventText[textLength - 1]))
        textLength--;
    eventText.truncate(textLength);
    eventText.replace("\r\n", " ");
    eventText.replace('\r', ' ');
    eventText.replace('\n', ' ');

    event->setText(limitLength(eventText, appendableLength));
}

bool TextFieldInputType::shouldRespectListAttribute()
{
    return InputType::themeSupportsDataListUI(this);
}

void TextFieldInputType::updatePlaceholderText()
{
    if (!supportsPlaceholder())
        return;
    String placeholderText = element()->strippedPlaceholder();
    if (placeholderText.isEmpty()) {
        if (m_placeholder) {
            m_placeholder->parentNode()->removeChild(m_placeholder.get(), ASSERT_NO_EXCEPTION);
            m_placeholder.clear();
        }
        return;
    }
    if (!m_placeholder) {
        m_placeholder = HTMLDivElement::create(element()->document());
        m_placeholder->setPseudo(AtomicString("-webkit-input-placeholder", AtomicString::ConstructFromLiteral));
        element()->userAgentShadowRoot()->insertBefore(m_placeholder, m_container ? m_container->nextSibling() : innerTextElement()->nextSibling(), ASSERT_NO_EXCEPTION);
    }
    m_placeholder->setInnerText(placeholderText, ASSERT_NO_EXCEPTION);
    element()->fixPlaceholderRenderer(m_placeholder.get(), m_container ? m_container.get() : m_innerText.get());
}

void TextFieldInputType::attach()
{
    InputType::attach();
    // If container exists, the container should not have any content data.
    ASSERT(!m_container || !m_container->renderStyle() || !m_container->renderStyle()->hasContent());

    element()->fixPlaceholderRenderer(m_placeholder.get(), m_container ? m_container.get() : m_innerText.get());
}

bool TextFieldInputType::appendFormData(FormDataList& list, bool multipart) const
{
    InputType::appendFormData(list, multipart);
    const AtomicString& dirnameAttrValue = element()->fastGetAttribute(dirnameAttr);
    if (!dirnameAttrValue.isNull())
        list.appendData(dirnameAttrValue, element()->directionForFormData());
    return true;
}

String TextFieldInputType::convertFromVisibleValue(const String& visibleValue) const
{
    return visibleValue;
}

void TextFieldInputType::subtreeHasChanged()
{
    ASSERT(element()->renderer());

    bool wasChanged = element()->wasChangedSinceLastFormControlChangeEvent();
    element()->setChangedSinceLastFormControlChangeEvent(true);

    // We don't need to call sanitizeUserInputValue() function here because
    // HTMLInputElement::handleBeforeTextInsertedEvent() has already called
    // sanitizeUserInputValue().
    // sanitizeValue() is needed because IME input doesn't dispatch BeforeTextInsertedEvent.
    element()->setValueFromRenderer(sanitizeValue(convertFromVisibleValue(element()->innerTextValue())));
    element()->updatePlaceholderVisibility(false);
    // Recalc for :invalid change.
    element()->setNeedsStyleRecalc();

    didSetValueByUserEdit(wasChanged ? ValueChangeStateChanged : ValueChangeStateNone);
}

void TextFieldInputType::didSetValueByUserEdit(ValueChangeState state)
{
    if (!element()->focused())
        return;
    if (Frame* frame = element()->document()->frame()) {
        if (state == ValueChangeStateNone)
            frame->editor().textFieldDidBeginEditing(element());
        frame->editor().textDidChangeInTextField(element());
    }
}

void TextFieldInputType::spinButtonStepDown()
{
    stepUpFromRenderer(-1);
}

void TextFieldInputType::spinButtonStepUp()
{
    stepUpFromRenderer(1);
}

void TextFieldInputType::updateInnerTextValue()
{
    if (!element()->suggestedValue().isNull()) {
        element()->setInnerTextValue(element()->suggestedValue());
        element()->updatePlaceholderVisibility(false);
    } else if (!element()->formControlValueMatchesRenderer()) {
        // Update the renderer value if the formControlValueMatchesRenderer() flag is false.
        // It protects an unacceptable renderer value from being overwritten with the DOM value.
        element()->setInnerTextValue(visibleValue());
        element()->updatePlaceholderVisibility(false);
    }
}

void TextFieldInputType::focusAndSelectSpinButtonOwner()
{
    RefPtr<HTMLInputElement> input(element());
    input->focus();
    input->select();
}

bool TextFieldInputType::shouldSpinButtonRespondToMouseEvents()
{
    return !element()->isDisabledOrReadOnly();
}

bool TextFieldInputType::shouldSpinButtonRespondToWheelEvents()
{
    return shouldSpinButtonRespondToMouseEvents() && element()->focused();
}

} // namespace WebCore
