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

#ifndef NumberInputType_h
#define NumberInputType_h

#include "TextFieldInputType.h"

namespace WebCore {

class NumberInputType : public TextFieldInputType {
public:
    static PassOwnPtr<InputType> create(HTMLInputElement*);

private:
    NumberInputType(HTMLInputElement* element) : TextFieldInputType(element) { }
    virtual void attach() OVERRIDE;
    virtual const AtomicString& formControlType() const OVERRIDE;
    virtual void setValue(const String&, bool valueChanged, TextFieldEventBehavior) OVERRIDE;
    virtual double valueAsDouble() const OVERRIDE;
    virtual void setValueAsDouble(double, TextFieldEventBehavior, ExceptionCode&) const OVERRIDE;
    virtual void setValueAsDecimal(const Decimal&, TextFieldEventBehavior, ExceptionCode&) const OVERRIDE;
    virtual bool typeMismatchFor(const String&) const OVERRIDE;
    virtual bool typeMismatch() const OVERRIDE;
    virtual bool sizeShouldIncludeDecoration(int defaultSize, int& preferredSize) const OVERRIDE;
    virtual bool isSteppable() const OVERRIDE;
    virtual StepRange createStepRange(AnyStepHandling) const OVERRIDE;
    virtual void handleKeydownEvent(KeyboardEvent*) OVERRIDE;
    virtual Decimal parseToNumber(const String&, const Decimal&) const OVERRIDE;
    virtual String serialize(const Decimal&) const OVERRIDE;
    virtual String localizeValue(const String&) const OVERRIDE;
    virtual String visibleValue() const OVERRIDE;
    virtual String convertFromVisibleValue(const String&) const OVERRIDE;
    virtual String sanitizeValue(const String&) const OVERRIDE;
    virtual bool hasBadInput() const OVERRIDE;
    virtual String badInputText() const OVERRIDE;
    virtual bool shouldRespectSpeechAttribute() OVERRIDE;
    virtual bool supportsPlaceholder() const OVERRIDE;
    virtual bool isNumberField() const OVERRIDE;
    virtual void minOrMaxAttributeChanged() OVERRIDE;
    virtual void stepAttributeChanged() OVERRIDE;
};

} // namespace WebCore

#endif // NumberInputType_h
