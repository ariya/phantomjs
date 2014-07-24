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

#ifndef CustomElementRegistry_h
#define CustomElementRegistry_h

#if ENABLE(CUSTOM_ELEMENTS)

#include "ContextDestructionObserver.h"
#include "ExceptionCode.h"
#include "QualifiedName.h"
#include "ScriptValue.h"
#include "Supplementable.h"
#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class CustomElementConstructor;
class Dictionary;
class Document;
class Element;
class ScriptExecutionContext;
class QualifiedName;

class CustomElementInvocation {
public:
    explicit CustomElementInvocation(PassRefPtr<Element>);
    ~CustomElementInvocation();

    Element* element() const { return m_element.get(); }

private:
    RefPtr<Element> m_element;
};

class CustomElementRegistry : public RefCounted<CustomElementRegistry> , public ContextDestructionObserver {
    WTF_MAKE_NONCOPYABLE(CustomElementRegistry); WTF_MAKE_FAST_ALLOCATED;
public:
    class CallbackDeliveryScope {
    public:
        CallbackDeliveryScope() { }
        ~CallbackDeliveryScope() { CustomElementRegistry::deliverAllLifecycleCallbacksIfNeeded(); }
    };

    explicit CustomElementRegistry(Document*);
    ~CustomElementRegistry();

    PassRefPtr<CustomElementConstructor> registerElement(WebCore::ScriptState*, const AtomicString& name, const Dictionary& options, ExceptionCode&);
    PassRefPtr<CustomElementConstructor> findFor(Element*) const;
    PassRefPtr<CustomElementConstructor> find(const QualifiedName& elementName, const QualifiedName& localName) const;
    PassRefPtr<Element> createElement(const QualifiedName& localName, const AtomicString& typeExtension) const;

    Document* document() const;

    void didGiveTypeExtension(Element*);
    void didCreateElement(Element*);

    static void deliverAllLifecycleCallbacks();
    static void deliverAllLifecycleCallbacksIfNeeded();

private:
    typedef HashMap<std::pair<QualifiedName, QualifiedName>, RefPtr<CustomElementConstructor> > ConstructorMap;
    typedef HashSet<AtomicString> NameSet;
    typedef ListHashSet<CustomElementRegistry*> InstanceSet;

    static bool isValidName(const AtomicString&);
    static InstanceSet& activeCustomElementRegistries();

    void activate(const CustomElementInvocation&);
    void deactivate();
    void deliverLifecycleCallbacks();

    ConstructorMap m_constructors;
    NameSet m_names;
    Vector<CustomElementInvocation> m_invocations;
};

inline void CustomElementRegistry::deliverAllLifecycleCallbacksIfNeeded()
{
    if (!activeCustomElementRegistries().isEmpty())
        deliverAllLifecycleCallbacks();
    ASSERT(activeCustomElementRegistries().isEmpty());
}

inline CustomElementRegistry::InstanceSet& CustomElementRegistry::activeCustomElementRegistries()
{
    DEFINE_STATIC_LOCAL(InstanceSet, activeInstances, ());
    return activeInstances;
}


} // namespace WebCore

#endif // ENABLE(CUSTOM_ELEMENTS)
#endif
