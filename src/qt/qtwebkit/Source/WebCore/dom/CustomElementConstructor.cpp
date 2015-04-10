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

#include "config.h"

#if ENABLE(CUSTOM_ELEMENTS)

#include "CustomElementConstructor.h"

#include "CustomElementHelpers.h"
#include "Document.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "SVGElement.h"
#include "SVGNames.h"
#include <wtf/Assertions.h>

namespace WebCore {

PassRefPtr<CustomElementConstructor> CustomElementConstructor::create(ScriptState* state, Document* document, const QualifiedName& typeName, const QualifiedName& localName, const ScriptValue& prototype)
{
    ASSERT(CustomElementHelpers::isValidPrototypeParameter(prototype, state));
    ASSERT(localName == typeName || localName == *CustomElementHelpers::findLocalName(prototype));
    RefPtr<CustomElementConstructor> created = adoptRef(new CustomElementConstructor(document, typeName, localName));
    if (!CustomElementHelpers::initializeConstructorWrapper(created.get(), prototype, state))
        return 0;
    return created.release();
}

CustomElementConstructor::CustomElementConstructor(Document* document, const QualifiedName& typeName, const QualifiedName& localName)
    : ContextDestructionObserver(document)
    , m_typeName(typeName)
    , m_localName(localName)
{
}

CustomElementConstructor::~CustomElementConstructor()
{
}

PassRefPtr<Element> CustomElementConstructor::createElement()
{
    RefPtr<Element> element = createElementInternal();
    if (element)
        document()->didCreateCustomElement(element.get(), this);
    return element.release();
}

PassRefPtr<Element> CustomElementConstructor::createElementInternal()
{
    if (!document())
        return 0;
    if (m_localName != m_typeName)
        return setTypeExtension(document()->createElement(m_localName, document()), m_typeName.localName());
    if (HTMLNames::xhtmlNamespaceURI == m_typeName.namespaceURI())
        return HTMLElement::create(m_typeName, document());
#if ENABLE(SVG)
    if (SVGNames::svgNamespaceURI == m_typeName.namespaceURI())
        return SVGElement::create(m_typeName, document());
#endif
    return Element::create(m_typeName, document());
}

PassRefPtr<Element> setTypeExtension(PassRefPtr<Element> element, const AtomicString& typeExtension)
{
    if (!typeExtension.isEmpty())
        element->setAttribute(HTMLNames::isAttr, typeExtension);
    return element;
}

}

#endif // ENABLE(CUSTOM_ELEMENTS)
