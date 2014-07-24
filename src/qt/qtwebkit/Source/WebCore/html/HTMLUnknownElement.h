/*
 * Copyright (c) 2011 Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLUnknownElement_h
#define HTMLUnknownElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLUnknownElement FINAL : public HTMLElement {
public:
    static PassRefPtr<HTMLUnknownElement> create(const QualifiedName& tagName, Document* document)
    {
        return adoptRef(new HTMLUnknownElement(tagName, document));
    }

    virtual bool isHTMLUnknownElement() const OVERRIDE { return true; }

private:
    HTMLUnknownElement(const QualifiedName& tagName, Document* document)
        : HTMLElement(tagName, document)
    {
    }
};

inline HTMLUnknownElement* toHTMLUnknownElement(HTMLElement* element)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!element || element->isHTMLUnknownElement());
    return static_cast<HTMLUnknownElement*>(element);
}

} // namespace

#endif
