/*
 * Copyright (c) 2012, Google Inc. All rights reserved.
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

#ifndef HTMLTemplateElement_h
#define HTMLTemplateElement_h

#if ENABLE(TEMPLATE_ELEMENT)

#include "DocumentFragment.h"
#include "HTMLElement.h"

namespace WebCore {

class HTMLTemplateElement FINAL : public HTMLElement {
public:
    static PassRefPtr<HTMLTemplateElement> create(const QualifiedName&, Document*);
    virtual ~HTMLTemplateElement();

    DocumentFragment* content() const;

private:
    virtual PassRefPtr<Node> cloneNode(bool deep) OVERRIDE;
    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;

    HTMLTemplateElement(const QualifiedName&, Document*);

    mutable RefPtr<DocumentFragment> m_content;
};

const HTMLTemplateElement* toHTMLTemplateElement(const Node*);

inline HTMLTemplateElement* toHTMLTemplateElement(Node* node)
{
    return const_cast<HTMLTemplateElement*>(toHTMLTemplateElement(static_cast<const Node*>(node)));
}

#ifdef NDEBUG

// The debug version of this, with assertions, is not inlined.
inline const HTMLTemplateElement* toHTMLTemplateElement(const Node* node)
{
    return static_cast<const HTMLTemplateElement*>(node);
}
#endif // NDEBUG

} // namespace WebCore

#endif // ENABLE(TEMPLATE_ELEMENT)

#endif // HTMLTemplateElement_h
