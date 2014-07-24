/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#include "CustomElementRegistry.h"

#include "CustomElementConstructor.h"
#include "CustomElementHelpers.h"
#include "Dictionary.h"
#include "Document.h"
#include "Element.h"
#include "HTMLNames.h"
#include "HTMLUnknownElement.h"
#include "RuntimeEnabledFeatures.h"
#include <wtf/ASCIICType.h>
#include <wtf/HashSet.h>

#if ENABLE(SVG)
#include "SVGNames.h"
#endif

#if ENABLE(MATHML)
#include "MathMLNames.h"
#endif

namespace WebCore {

CustomElementInvocation::CustomElementInvocation(PassRefPtr<Element> element)
    : m_element(element)
{
}

CustomElementInvocation::~CustomElementInvocation()
{
}

CustomElementRegistry::CustomElementRegistry(Document* document)
    : ContextDestructionObserver(document)
{
}

CustomElementRegistry::~CustomElementRegistry()
{
    deactivate();
}

static inline bool nameIncludesHyphen(const AtomicString& name)
{
    size_t hyphenPosition = name.find('-');
    return (hyphenPosition != notFound);
}

bool CustomElementRegistry::isValidName(const AtomicString& name)
{
    if (!nameIncludesHyphen(name))
        return false;

    DEFINE_STATIC_LOCAL(Vector<AtomicString>, reservedNames, ());
    if (reservedNames.isEmpty()) {
#if ENABLE(MATHML)
        reservedNames.append(MathMLNames::annotation_xmlTag.localName());
#endif

#if ENABLE(SVG)
        reservedNames.append(SVGNames::color_profileTag.localName());
        reservedNames.append(SVGNames::font_faceTag.localName());
        reservedNames.append(SVGNames::font_face_srcTag.localName());
        reservedNames.append(SVGNames::font_face_uriTag.localName());
        reservedNames.append(SVGNames::font_face_formatTag.localName());
        reservedNames.append(SVGNames::font_face_nameTag.localName());
        reservedNames.append(SVGNames::missing_glyphTag.localName());
#endif
    }

    if (notFound != reservedNames.find(name))
        return false;

    return Document::isValidName(name.string());
}

PassRefPtr<CustomElementConstructor> CustomElementRegistry::registerElement(ScriptState* state, const AtomicString& name, const Dictionary& options, ExceptionCode& ec)
{
    RefPtr<CustomElementRegistry> protect(this);

    if (!CustomElementHelpers::isFeatureAllowed(state))
        return 0;

    AtomicString lowerName = name.lower();
    if (!isValidName(lowerName)) {
        ec = INVALID_CHARACTER_ERR;
        return 0;
    }
        
    ScriptValue prototypeValue;
    if (!options.get("prototype", prototypeValue)) {
        // FIXME: Implement the default value handling.
        // Currently default value of the "prototype" parameter, which
        // is HTMLSpanElement.prototype, has an ambiguity about its
        // behavior. The spec should be fixed before WebKit implements
        // it. https://www.w3.org/Bugs/Public/show_bug.cgi?id=20801
        ec = INVALID_STATE_ERR;
        return 0;
    }

    AtomicString namespaceURI;
    if (!CustomElementHelpers::isValidPrototypeParameter(prototypeValue, state, namespaceURI)) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    if (m_names.contains(lowerName)) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    const QualifiedName* localNameFound = CustomElementHelpers::findLocalName(prototypeValue);
    QualifiedName typeName(nullAtom, lowerName, namespaceURI);
    QualifiedName localNameToUse = localNameFound ? *localNameFound : typeName;
    if (find(typeName, localNameToUse)) {
        ec = INVALID_STATE_ERR;
        return 0;
    }
    
    // A script execution could happen in isValidPrototypeParameter(), which kills the document.
    if (!document()) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    RefPtr<CustomElementConstructor> constructor = CustomElementConstructor::create(state, document(), typeName, localNameToUse, prototypeValue);
    if (!constructor) {
        ec = INVALID_STATE_ERR;
        return 0;
    }
        
    m_constructors.add(std::make_pair(constructor->typeName(), constructor->localName()), constructor);
    m_names.add(lowerName);

    return constructor;
}

PassRefPtr<CustomElementConstructor> CustomElementRegistry::findFor(Element* element) const
{
    ASSERT(element->document()->registry() == this);

    // Most elements can be rejected this quick screening.
    if (!nameIncludesHyphen(element->tagName()) && !element->hasAttribute(HTMLNames::isAttr))
        return 0;

    QualifiedName idValue(nullAtom, element->getAttribute(HTMLNames::isAttr), HTMLNames::xhtmlNamespaceURI);
    return find(idValue, element->tagQName());
}

PassRefPtr<CustomElementConstructor> CustomElementRegistry::find(const QualifiedName& typeName, const QualifiedName& localName) const
{
    ConstructorMap::const_iterator found = m_constructors.end();
    if (!typeName.localName().isEmpty())
        found = m_constructors.find(std::make_pair(typeName, localName));
    if (found == m_constructors.end())
        found = m_constructors.find(std::make_pair(localName, localName));
    if (found == m_constructors.end())
        return 0;
    return found->value;
}

PassRefPtr<Element> CustomElementRegistry::createElement(const QualifiedName& localName, const AtomicString& typeExtension) const
{
    const QualifiedName& typeName = QualifiedName(nullAtom, typeExtension, localName.namespaceURI());
    if (RefPtr<CustomElementConstructor> found = find(typeName, localName)) {
        RefPtr<Element> created = found->createElement();
        if (!typeName.localName().isEmpty() && localName != typeName)
            return setTypeExtension(created, typeExtension);
        return created.release();
    }

    return 0;
}

void CustomElementRegistry::didGiveTypeExtension(Element* element)
{
    RefPtr<CustomElementConstructor> constructor = findFor(element);
    if (!constructor || !constructor->isExtended())
        return;
    activate(CustomElementInvocation(element));
}

void CustomElementRegistry::didCreateElement(Element* element)
{
    activate(CustomElementInvocation(element));
}

void CustomElementRegistry::activate(const CustomElementInvocation& invocation)
{
    bool wasInactive = m_invocations.isEmpty();
    m_invocations.append(invocation);
    if (wasInactive)
        activeCustomElementRegistries().add(this);
}

void CustomElementRegistry::deactivate()
{
    ASSERT(m_invocations.isEmpty());
    if (activeCustomElementRegistries().contains(this))
        activeCustomElementRegistries().remove(this);
}

inline Document* CustomElementRegistry::document() const
{
    return toDocument(m_scriptExecutionContext);
}

void CustomElementRegistry::deliverLifecycleCallbacks()
{
    ASSERT(!m_invocations.isEmpty());

    if (!m_invocations.isEmpty()) {
        Vector<CustomElementInvocation> invocations;
        m_invocations.swap(invocations);
        CustomElementHelpers::invokeReadyCallbacksIfNeeded(m_scriptExecutionContext, invocations);
    }

    ASSERT(m_invocations.isEmpty());
    deactivate();
}

void CustomElementRegistry::deliverAllLifecycleCallbacks()
{
    while (!activeCustomElementRegistries().isEmpty()) {
        Vector<RefPtr<CustomElementRegistry> > registries;
        copyToVector(activeCustomElementRegistries(), registries);
        activeCustomElementRegistries().clear();
        for (size_t i = 0; i < registries.size(); ++i)
            registries[i]->deliverLifecycleCallbacks();
    }
}

}

#endif // ENABLE(CUSTOM_ELEMENTS)
