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

#ifndef BaseDateAndTimeInputType_h
#define BaseDateAndTimeInputType_h

#include "DateComponents.h"
#include "TextFieldInputType.h"
#include <wtf/unicode/Unicode.h>

namespace WebCore {

// A super class of date, datetime, datetime-local, month, time, and week types.
class BaseDateAndTimeInputType : public TextFieldInputType {
protected:
    BaseDateAndTimeInputType(HTMLInputElement* element) : TextFieldInputType(element) { }
    virtual double parseToDouble(const String&, double) const;
    virtual bool parseToDateComponents(const String&, DateComponents*) const;
    String serializeWithComponents(const DateComponents&) const;

private:
    virtual bool parseToDateComponentsInternal(const UChar*, unsigned length, DateComponents*) const = 0;
    virtual bool setMillisecondToDateComponents(double, DateComponents*) const = 0;
    virtual DateComponents::Type dateType() const = 0;
    virtual double valueAsDate() const;
    virtual void setValueAsDate(double, ExceptionCode&) const;
    virtual double valueAsNumber() const;
    virtual void setValueAsNumber(double, ExceptionCode&) const;
    virtual bool typeMismatchFor(const String&) const;
    virtual bool typeMismatch() const;
    virtual bool rangeUnderflow(const String&) const;
    virtual bool rangeOverflow(const String&) const;
    virtual bool supportsRangeLimitation() const;
    virtual double defaultValueForStepUp() const;
    virtual bool isSteppable() const;
    virtual bool stepMismatch(const String&, double) const;
    virtual double stepBase() const;
    virtual void handleKeydownEvent(KeyboardEvent*);
    virtual void handleWheelEvent(WheelEvent*);
    virtual String serialize(double) const;
    virtual String serializeWithMilliseconds(double) const;
    virtual String visibleValue() const;
    virtual String convertFromVisibleValue(const String&) const;
};

} // namespace WebCore

#endif // BaseDateAndTimeInputType_h
