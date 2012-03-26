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
    virtual const AtomicString& formControlType() const;
    virtual double valueAsNumber() const;
    virtual void setValueAsNumber(double, ExceptionCode&) const;
    virtual bool typeMismatchFor(const String&) const;
    virtual bool typeMismatch() const;
    virtual bool rangeUnderflow(const String&) const;
    virtual bool rangeOverflow(const String&) const;
    virtual bool supportsRangeLimitation() const;
    virtual double minimum() const;
    virtual double maximum() const;
    virtual bool isSteppable() const;
    virtual bool stepMismatch(const String&, double) const;
    virtual double stepBase() const;
    virtual double stepBaseWithDecimalPlaces(unsigned*) const;
    virtual double defaultStep() const;
    virtual double stepScaleFactor() const;
    virtual void handleKeydownEvent(KeyboardEvent*);
    virtual void handleWheelEvent(WheelEvent*);
    virtual double parseToDouble(const String&, double) const;
    virtual double parseToDoubleWithDecimalPlaces(const String&, double, unsigned*) const;
    virtual String serialize(double) const;
    virtual double acceptableError(double) const;
    virtual void handleBlurEvent();
    virtual String visibleValue() const;
    virtual String convertFromVisibleValue(const String&) const;
    virtual bool isAcceptableValue(const String&);
    virtual String sanitizeValue(const String&);
    virtual bool hasUnacceptableValue();
    virtual bool shouldRespectSpeechAttribute();
    virtual bool isNumberField() const;
};

} // namespace WebCore

#endif // NumberInputType_h
