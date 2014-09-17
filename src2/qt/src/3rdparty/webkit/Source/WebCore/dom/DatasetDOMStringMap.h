/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DatasetDOMStringMap_h
#define DatasetDOMStringMap_h

#include "DOMStringMap.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class Element;

class DatasetDOMStringMap : public DOMStringMap {
public:
    static PassOwnPtr<DatasetDOMStringMap> create(Element* element)
    {
        return adoptPtr(new DatasetDOMStringMap(element));
    }

    virtual void ref();
    virtual void deref();

    virtual void getNames(Vector<String>&);
    virtual String item(const String& name);
    virtual bool contains(const String& name);
    virtual void setItem(const String& name, const String& value, ExceptionCode&);
    virtual void deleteItem(const String& name, ExceptionCode&);

    virtual Element* element() { return m_element; }

private:
    DatasetDOMStringMap(Element* element)
        : m_element(element)
    {
    }

    Element* m_element;
};

} // namespace WebCore

#endif // DatasetDOMStringMap_h
