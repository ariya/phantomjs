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

#ifndef BaseCheckableInputType_h
#define BaseCheckableInputType_h

#include "InputType.h"

namespace WebCore {

// Base of checkbox and radio types.
class BaseCheckableInputType : public InputType {
protected:
    BaseCheckableInputType(HTMLInputElement* element) : InputType(element) { }
    virtual void handleKeydownEvent(KeyboardEvent*);

private:
    virtual FormControlState saveFormControlState() const OVERRIDE;
    virtual void restoreFormControlState(const FormControlState&) OVERRIDE;
    virtual bool appendFormData(FormDataList&, bool) const OVERRIDE;
    virtual void handleKeypressEvent(KeyboardEvent*) OVERRIDE;
    virtual bool canSetStringValue() const OVERRIDE;
    virtual void accessKeyAction(bool sendMouseEvents) OVERRIDE;
    virtual String fallbackValue() const OVERRIDE;
    virtual bool storesValueSeparateFromAttribute() OVERRIDE;
    virtual void setValue(const String&, bool, TextFieldEventBehavior) OVERRIDE;
    virtual bool isCheckable() OVERRIDE;
};

} // namespace WebCore

#endif // BaseCheckableInputType_h
