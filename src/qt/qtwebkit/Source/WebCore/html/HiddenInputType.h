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

#ifndef HiddenInputType_h
#define HiddenInputType_h

#include "InputType.h"

namespace WebCore {

class HiddenInputType : public InputType {
public:
    static PassOwnPtr<InputType> create(HTMLInputElement*);

private:
    HiddenInputType(HTMLInputElement* element) : InputType(element) { }
    virtual const AtomicString& formControlType() const OVERRIDE;
    virtual FormControlState saveFormControlState() const OVERRIDE;
    virtual void restoreFormControlState(const FormControlState&) OVERRIDE;
    virtual bool supportsValidation() const OVERRIDE;
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) const OVERRIDE;
    virtual void accessKeyAction(bool sendMouseEvents) OVERRIDE;
    virtual bool rendererIsNeeded() OVERRIDE;
    virtual bool storesValueSeparateFromAttribute() OVERRIDE;
    virtual bool isHiddenType() const OVERRIDE;
    virtual bool supportLabels() const OVERRIDE { return false; }
    virtual bool shouldRespectHeightAndWidthAttributes() OVERRIDE;
    virtual void setValue(const String&, bool, TextFieldEventBehavior) OVERRIDE;
    virtual bool appendFormData(FormDataList&, bool) const OVERRIDE;
};

} // namespace WebCore

#endif // HiddenInputType_h
