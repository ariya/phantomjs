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

#ifndef RangeInputType_h
#define RangeInputType_h

#include "InputType.h"

namespace WebCore {

class SliderThumbElement;

class RangeInputType : public InputType {
public:
    static PassOwnPtr<InputType> create(HTMLInputElement*);

private:
    RangeInputType(HTMLInputElement* element) : InputType(element) { }
    virtual bool isRangeControl() const;
    virtual const AtomicString& formControlType() const;
    virtual double valueAsNumber() const;
    virtual void setValueAsNumber(double, ExceptionCode&) const;
    virtual bool supportsRequired() const;
    virtual bool rangeUnderflow(const String&) const;
    virtual bool rangeOverflow(const String&) const;
    virtual bool supportsRangeLimitation() const;
    virtual double minimum() const;
    virtual double maximum() const;
    virtual bool isSteppable() const;
    virtual bool stepMismatch(const String&, double) const;
    virtual double stepBase() const;
    virtual double defaultStep() const;
    virtual double stepScaleFactor() const;
    virtual void handleMouseDownEvent(MouseEvent*);
    virtual void handleKeydownEvent(KeyboardEvent*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) const;
    virtual void createShadowSubtree();
    virtual double parseToDouble(const String&, double) const;
    virtual String serialize(double) const;
    virtual void accessKeyAction(bool sendToAnyElement);
    virtual void minOrMaxAttributeChanged();
    virtual void valueChanged();
    virtual String fallbackValue();
    virtual String sanitizeValue(const String& proposedValue);
    virtual bool shouldRespectListAttribute();

    SliderThumbElement* shadowSliderThumb() const;
};

} // namespace WebCore

#endif // RangeInputType_h
