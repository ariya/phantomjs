/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef ProgressShadowElement_h
#define ProgressShadowElement_h

#include "HTMLDivElement.h"
#include <wtf/Forward.h>

namespace WebCore {

class HTMLProgressElement;

class ProgressShadowElement : public HTMLDivElement {
public:
    ProgressShadowElement(Document*);
    HTMLProgressElement* progressElement() const;

private:
    virtual bool rendererIsNeeded(RenderStyle*);
};

class ProgressBarElement : public ProgressShadowElement {
public:
    ProgressBarElement(Document* document) 
        : ProgressShadowElement(document)
    {
    }

    static PassRefPtr<ProgressBarElement> create(Document*);
    virtual const AtomicString& shadowPseudoId() const;
};

inline PassRefPtr<ProgressBarElement> ProgressBarElement::create(Document* document)
{
    return adoptRef(new ProgressBarElement(document));
}


class ProgressValueElement : public ProgressShadowElement {
public:
    ProgressValueElement(Document* document) 
        : ProgressShadowElement(document)
    {
    }

    virtual const AtomicString& shadowPseudoId() const;
    static PassRefPtr<ProgressValueElement> create(Document*);
    void setWidthPercentage(double);
};

inline PassRefPtr<ProgressValueElement> ProgressValueElement::create(Document* document)
{
    return adoptRef(new ProgressValueElement(document));
}

}

#endif
