/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ShadowElement_h
#define ShadowElement_h

#include "HTMLDivElement.h"
#include "HTMLInputElement.h"

namespace WebCore {

template<class BaseElement>
class ShadowElement : public BaseElement {
protected:
    ShadowElement(const QualifiedName& name, HTMLElement* shadowParent)
        : BaseElement(name, shadowParent->document())
    {
        BaseElement::setShadowHost(shadowParent);
    }

    ShadowElement(const QualifiedName& name, HTMLElement* shadowParent, HTMLFormElement* form, bool createdByParser)
        : BaseElement(name, shadowParent->document(), form, createdByParser)
    {
        BaseElement::setShadowHost(shadowParent);
    }

public:
    virtual void detach();
};

template<class BaseElement>
void ShadowElement<BaseElement>::detach()
{
    BaseElement::detach();
    // FIXME: Remove once shadow DOM uses Element::setShadowRoot().
    BaseElement::setShadowHost(0);
}

class ShadowInputElement : public ShadowElement<HTMLInputElement> {
public:
    static PassRefPtr<ShadowInputElement> create(HTMLElement*);
protected:
    ShadowInputElement(HTMLElement*);
};

} // namespace WebCore

#endif // ShadowElement_h
