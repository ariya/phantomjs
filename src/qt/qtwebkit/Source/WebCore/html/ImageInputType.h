/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

#ifndef ImageInputType_h
#define ImageInputType_h

#include "BaseButtonInputType.h"
#include "IntPoint.h"

namespace WebCore {

class ImageInputType : public BaseButtonInputType {
public:
    static PassOwnPtr<InputType> create(HTMLInputElement*);

private:
    ImageInputType(HTMLInputElement*);
    virtual const AtomicString& formControlType() const OVERRIDE;
    virtual bool isFormDataAppendable() const OVERRIDE;
    virtual bool appendFormData(FormDataList&, bool) const OVERRIDE;
    virtual bool supportsValidation() const OVERRIDE;
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) const OVERRIDE;
    virtual void handleDOMActivateEvent(Event*) OVERRIDE;
    virtual void altAttributeChanged() OVERRIDE;
    virtual void srcAttributeChanged() OVERRIDE;
    virtual void attach() OVERRIDE;
    virtual bool shouldRespectAlignAttribute() OVERRIDE;
    virtual bool canBeSuccessfulSubmitButton() OVERRIDE;
    virtual bool isImageButton() const OVERRIDE;
    virtual bool isEnumeratable() OVERRIDE;
    virtual bool shouldRespectHeightAndWidthAttributes() OVERRIDE;
    virtual unsigned height() const OVERRIDE;
    virtual unsigned width() const OVERRIDE;

    IntPoint m_clickLocation; // Valid only during HTMLFormElement::prepareForSubmission().
};

} // namespace WebCore

#endif // ImageInputType_h
