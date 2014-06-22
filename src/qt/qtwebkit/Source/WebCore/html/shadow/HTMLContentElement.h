/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef HTMLContentElement_h
#define HTMLContentElement_h

#include "CSSSelectorList.h"
#include "InsertionPoint.h"

namespace WebCore {

#if ENABLE(SHADOW_DOM)

class HTMLContentElement FINAL : public InsertionPoint {
public:
    static const QualifiedName& contentTagName(Document*);
    static PassRefPtr<HTMLContentElement> create(const QualifiedName&, Document*);
    static PassRefPtr<HTMLContentElement> create(Document*);

    virtual ~HTMLContentElement();

    void setSelect(const AtomicString&);
    const AtomicString& select() const;

    virtual MatchType matchTypeFor(Node*) OVERRIDE;
    virtual const CSSSelectorList& selectorList() OVERRIDE;
    virtual Type insertionPointType() const OVERRIDE { return HTMLContentElementType; }
    virtual bool canAffectSelector() const OVERRIDE { return true; }
    virtual bool isSelectValid();

protected:
    HTMLContentElement(const QualifiedName&, Document*);

private:
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    void ensureSelectParsed();
    bool validateSelect() const;

    bool m_shouldParseSelectorList;
    bool m_isValidSelector;
    CSSSelectorList m_selectorList;
};

inline void HTMLContentElement::setSelect(const AtomicString& selectValue)
{
    setAttribute(HTMLNames::selectAttr, selectValue);
    m_shouldParseSelectorList = true;
}

inline const CSSSelectorList& HTMLContentElement::selectorList()
{
    ensureSelectParsed();
    return m_selectorList;
}

inline bool isHTMLContentElement(const Node* node)
{
    ASSERT(node);
    return node->isInsertionPoint() && toInsertionPoint(node)->insertionPointType() == InsertionPoint::HTMLContentElementType;
}

inline HTMLContentElement* toHTMLContentElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLContentElement(node));
    return static_cast<HTMLContentElement*>(node);
}

#endif // if ENABLE(SHADOW_DOM)

}

#endif
