/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef CustomElementConstructor_h
#define CustomElementConstructor_h

#if ENABLE(CUSTOM_ELEMENTS)

#include "ContextDestructionObserver.h"
#include "Document.h"
#include "QualifiedName.h"
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class Document;
class Element;
class ScriptState;
class ScriptValue;

PassRefPtr<Element> setTypeExtension(PassRefPtr<Element>, const AtomicString& typeExtension);

class CustomElementConstructor : public RefCounted<CustomElementConstructor> , public ContextDestructionObserver {
public:
    static PassRefPtr<CustomElementConstructor> create(ScriptState*, Document*, const QualifiedName& typeName, const QualifiedName& localName, const ScriptValue&);

    virtual ~CustomElementConstructor();

    Document* document() const { return static_cast<Document*>(m_scriptExecutionContext); }
    const QualifiedName& typeName() const { return m_typeName; }
    const QualifiedName& localName() const { return m_localName; }
    bool isExtended() const { return m_typeName != m_localName; }

    PassRefPtr<Element> createElement();

private:
    CustomElementConstructor(Document*, const QualifiedName& typeName, const QualifiedName& localName);

    PassRefPtr<Element> createElementInternal();

    QualifiedName m_typeName;
    QualifiedName m_localName;
};

}

#endif // ENABLE(CUSTOM_ELEMENTS)

#endif // CustomElementConstructor_h
