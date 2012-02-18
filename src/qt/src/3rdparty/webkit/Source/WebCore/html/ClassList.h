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
#include "ExceptionCode.h"
#include "SpaceSplitString.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class Element;

class ClassList : public DOMTokenList {
public:
    static PassOwnPtr<ClassList> create(Element* element)
    {
        return adoptPtr(new ClassList(element));
    }

    virtual void ref();
    virtual void deref();

    virtual unsigned length() const;
    virtual const AtomicString item(unsigned index) const;
    virtual bool contains(const AtomicString&, ExceptionCode&) const;
    virtual void add(const AtomicString&, ExceptionCode&);
    virtual void remove(const AtomicString&, ExceptionCode&);
    virtual bool toggle(const AtomicString&, ExceptionCode&);
    virtual String toString() const;

    virtual Element* element() { return m_element; }

    void reset(const String&);

private:
    ClassList(Element*);

    void addInternal(const AtomicString&);
    bool containsInternal(const AtomicString&) const;
    void removeInternal(const AtomicString&);

    const SpaceSplitString& classNames() const;

    Element* m_element;
    SpaceSplitString m_classNamesForQuirksMode;
};

} // namespace WebCore

#endif // ClassList_h
