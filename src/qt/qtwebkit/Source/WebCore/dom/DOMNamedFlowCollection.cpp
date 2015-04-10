/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "config.h"
#include "DOMNamedFlowCollection.h"

#include "NamedFlowCollection.h"
#include "WebKitNamedFlow.h"

namespace WebCore {

DOMNamedFlowCollection::DOMNamedFlowCollection(const Vector<WebKitNamedFlow*>& namedFlows)
{
    for (Vector<WebKitNamedFlow*>::const_iterator it = namedFlows.begin(); it != namedFlows.end(); ++it)
        m_namedFlows.add(*it);
}

unsigned long DOMNamedFlowCollection::length() const
{
    return m_namedFlows.size();
}

PassRefPtr<WebKitNamedFlow> DOMNamedFlowCollection::item(unsigned long index) const
{
    if (index >= static_cast<unsigned long>(m_namedFlows.size()))
        return 0;
    DOMNamedFlowSet::const_iterator it = m_namedFlows.begin();
    for (unsigned long i = 0; i < index; ++i)
        ++it;
    return *it;
}

PassRefPtr<WebKitNamedFlow> DOMNamedFlowCollection::namedItem(const AtomicString& name) const
{
    DOMNamedFlowSet::const_iterator it = m_namedFlows.find<String, DOMNamedFlowHashTranslator>(name);
    if (it != m_namedFlows.end())
        return *it;
    return 0;
}

bool DOMNamedFlowCollection::hasNamedItem(const AtomicString& name) const
{
    return namedItem(name);
}

// The HashFunctions object used by the HashSet to compare between RefPtr<NamedFlows>.
// It is safe to set safeToCompareToEmptyOrDeleted because the HashSet will never contain null pointers or deleted values.
struct DOMNamedFlowCollection::DOMNamedFlowHashFunctions {
    static unsigned hash(PassRefPtr<WebKitNamedFlow> key) { return DefaultHash<String>::Hash::hash(key->name()); }
    static bool equal(PassRefPtr<WebKitNamedFlow> a, PassRefPtr<WebKitNamedFlow> b) { return a->name() == b->name(); }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

// The HashTranslator is used to lookup a RefPtr<NamedFlow> in the set using a name.
struct DOMNamedFlowCollection::DOMNamedFlowHashTranslator {
    static unsigned hash(const String& key) { return DefaultHash<String>::Hash::hash(key); }
    static bool equal(PassRefPtr<WebKitNamedFlow> a, const String& b) { return a->name() == b; }
};
} // namespace WebCore



