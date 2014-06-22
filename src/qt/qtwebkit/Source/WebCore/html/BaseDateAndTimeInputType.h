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

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
#include "DateComponents.h"
#include "InputType.h"

namespace WebCore {

// A super class of date, datetime, datetime-local, month, time, and week types.
class BaseDateAndTimeInputType : public InputType {
protected:
    BaseDateAndTimeInputType(HTMLInputElement* element) : InputType(element) { }
    virtual Decimal parseToNumber(const String&, const Decimal&) const OVERRIDE;
    virtual bool parseToDateComponents(const String&, DateComponents*) const OVERRIDE;
    virtual String sanitizeValue(const String&) const OVERRIDE;
    virtual String serialize(const Decimal&) const OVERRIDE;
    String serializeWithComponents(const DateComponents&) const;
    virtual bool setMillisecondToDateComponents(double, DateComponents*) const = 0;
    virtual String visibleValue() const OVERRIDE;

private:
    virtual bool parseToDateComponentsInternal(const UChar*, unsigned length, DateComponents*) const = 0;
    virtual DateComponents::Type dateType() const = 0;
    virtual double valueAsDate() const OVERRIDE;
    virtual void setValueAsDate(double, ExceptionCode&) const OVERRIDE;
    virtual double valueAsDouble() const OVERRIDE;
    virtual void setValueAsDecimal(const Decimal&, TextFieldEventBehavior, ExceptionCode&) const OVERRIDE;
    virtual bool typeMismatchFor(const String&) const OVERRIDE;
    virtual bool typeMismatch() const OVERRIDE;
    virtual bool valueMissing(const String&) const OVERRIDE;
    virtual Decimal defaultValueForStepUp() const OVERRIDE;
    virtual bool isSteppable() const OVERRIDE;
    virtual String serializeWithMilliseconds(double) const;
    virtual String localizeValue(const String&) const OVERRIDE;
    virtual bool supportsReadOnly() const OVERRIDE;
    virtual bool shouldRespectListAttribute() OVERRIDE;
};

} // namespace WebCore
#endif
#endif // BaseDateAndTimeInputType_h
