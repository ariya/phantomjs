/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef TextFieldInputType_h
#define TextFieldInputType_h

#include "InputType.h"
#include "SpinButtonElement.h"

namespace WebCore {

class FormDataList; 

// The class represents types of which UI contain text fields.
// It supports not only the types for BaseTextInputType but also type=number.
class TextFieldInputType : public InputType, protected SpinButtonElement::SpinButtonOwner {
protected:
    TextFieldInputType(HTMLInputElement*);
    virtual ~TextFieldInputType();
    virtual bool canSetSuggestedValue() OVERRIDE;
    virtual void handleKeydownEvent(KeyboardEvent*) OVERRIDE;
    void handleKeydownEventForSpinButton(KeyboardEvent*);

    virtual HTMLElement* containerElement() const OVERRIDE;
    virtual HTMLElement* innerBlockElement() const OVERRIDE;
    virtual HTMLElement* innerTextElement() const OVERRIDE;
    virtual HTMLElement* innerSpinButtonElement() const OVERRIDE;
#if ENABLE(INPUT_SPEECH)
    virtual HTMLElement* speechButtonElement() const OVERRIDE;
#endif

protected:
    virtual void attach() OVERRIDE;
    virtual bool needsContainer() const;
    virtual bool shouldHaveSpinButton() const;
    virtual void createShadowSubtree() OVERRIDE;
    virtual void destroyShadowSubtree() OVERRIDE;
    virtual void attributeChanged() OVERRIDE;
    virtual void disabledAttributeChanged() OVERRIDE;
    virtual void readonlyAttributeChanged() OVERRIDE;
    virtual bool supportsReadOnly() const OVERRIDE;
    virtual void handleBlurEvent() OVERRIDE;
    virtual void setValue(const String&, bool valueChanged, TextFieldEventBehavior) OVERRIDE;
    virtual void updateInnerTextValue() OVERRIDE;

    virtual String convertFromVisibleValue(const String&) const;
    enum ValueChangeState {
        ValueChangeStateNone,
        ValueChangeStateChanged
    };
    virtual void didSetValueByUserEdit(ValueChangeState);

private:
    virtual bool isKeyboardFocusable(KeyboardEvent*) const OVERRIDE;
    virtual bool isMouseFocusable() const OVERRIDE;
    virtual bool isTextField() const OVERRIDE;
    virtual bool valueMissing(const String&) const OVERRIDE;
    virtual void handleBeforeTextInsertedEvent(BeforeTextInsertedEvent*) OVERRIDE;
    virtual void forwardEvent(Event*) OVERRIDE;
    virtual bool shouldSubmitImplicitly(Event*) OVERRIDE;
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) const OVERRIDE;
    virtual bool shouldUseInputMethod() const OVERRIDE;
    virtual String sanitizeValue(const String&) const OVERRIDE;
    virtual bool shouldRespectListAttribute() OVERRIDE;
    virtual HTMLElement* placeholderElement() const OVERRIDE;
    virtual void updatePlaceholderText() OVERRIDE;
    virtual bool appendFormData(FormDataList&, bool multipart) const OVERRIDE;
    virtual void subtreeHasChanged() OVERRIDE;

    // SpinButtonElement::SpinButtonOwner functions.
    virtual void focusAndSelectSpinButtonOwner() OVERRIDE;
    virtual bool shouldSpinButtonRespondToMouseEvents() OVERRIDE;
    virtual bool shouldSpinButtonRespondToWheelEvents() OVERRIDE;
    virtual void spinButtonStepDown() OVERRIDE;
    virtual void spinButtonStepUp() OVERRIDE;

    RefPtr<HTMLElement> m_container;
    RefPtr<HTMLElement> m_innerBlock;
    RefPtr<HTMLElement> m_innerText;
    RefPtr<HTMLElement> m_placeholder;
    RefPtr<SpinButtonElement> m_innerSpinButton;
#if ENABLE(INPUT_SPEECH)
    RefPtr<HTMLElement> m_speechButton;
#endif
};

} // namespace WebCore

#endif // TextFieldInputType_h
