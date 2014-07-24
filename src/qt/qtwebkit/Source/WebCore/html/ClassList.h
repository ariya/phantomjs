/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ClassList_h
#define ClassList_h

#include "DOMTokenList.h"
#include "Element.h"
#include "HTMLNames.h"
#include "SpaceSplitString.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class Element;

typedef int ExceptionCode;

class ClassList : public DOMTokenList {
public:
    static PassOwnPtr<ClassList> create(Element* element)
    {
        return adoptPtr(new ClassList(element));
    }

    virtual void ref() OVERRIDE;
    virtual void deref() OVERRIDE;

    virtual unsigned length() const OVERRIDE;
    virtual const AtomicString item(unsigned index) const OVERRIDE;

    virtual Element* element() OVERRIDE { return m_element; }

    void clearValueForQuirksMode() { m_classNamesForQuirksMode = nullptr; }

private:
    ClassList(Element*);

    virtual bool containsInternal(const AtomicString&) const OVERRIDE;

    const SpaceSplitString& classNames() const;

    virtual AtomicString value() const OVERRIDE { return m_element->getAttribute(HTMLNames::classAttr); }
    virtual void setValue(const AtomicString& value) OVERRIDE { m_element->setAttribute(HTMLNames::classAttr, value); }

    Element* m_element;
    mutable OwnPtr<SpaceSplitString> m_classNamesForQuirksMode;
};

} // namespace WebCore

#endif // ClassList_h
