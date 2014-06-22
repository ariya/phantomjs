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
#include "NamedFlowCollection.h"

#include "DOMNamedFlowCollection.h"
#include "Document.h"
#include "InspectorInstrumentation.h"
#include "WebKitNamedFlow.h"

#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

NamedFlowCollection::NamedFlowCollection(Document* document)
    : ContextDestructionObserver(document)
{
}

Vector<RefPtr<WebKitNamedFlow> > NamedFlowCollection::namedFlows()
{
    Vector<RefPtr<WebKitNamedFlow> > namedFlows;

    for (NamedFlowSet::iterator it = m_namedFlows.begin(); it != m_namedFlows.end(); ++it) {
        if ((*it)->flowState() == WebKitNamedFlow::FlowStateNull)
            continue;

        namedFlows.append(RefPtr<WebKitNamedFlow>(*it));
    }

    return namedFlows;
}

WebKitNamedFlow* NamedFlowCollection::flowByName(const String& flowName)
{
    NamedFlowSet::iterator it = m_namedFlows.find<String, NamedFlowHashTranslator>(flowName);
    if (it == m_namedFlows.end() || (*it)->flowState() == WebKitNamedFlow::FlowStateNull)
        return 0;

    return *it;
}

PassRefPtr<WebKitNamedFlow> NamedFlowCollection::ensureFlowWithName(const String& flowName)
{
    NamedFlowSet::iterator it = m_namedFlows.find<String, NamedFlowHashTranslator>(flowName);
    if (it != m_namedFlows.end()) {
        WebKitNamedFlow* namedFlow = *it;
        ASSERT(namedFlow->flowState() == WebKitNamedFlow::FlowStateNull);

        return namedFlow;
    }

    RefPtr<WebKitNamedFlow> newFlow = WebKitNamedFlow::create(this, flowName);
    m_namedFlows.add(newFlow.get());

    InspectorInstrumentation::didCreateNamedFlow(document(), newFlow.get());

    return newFlow.release();
}

void NamedFlowCollection::discardNamedFlow(WebKitNamedFlow* namedFlow)
{
    // The document is not valid anymore so the collection will be destroyed anyway.
    if (!document())
        return;

    ASSERT(namedFlow->flowState() == WebKitNamedFlow::FlowStateNull);
    ASSERT(m_namedFlows.contains(namedFlow));

    InspectorInstrumentation::willRemoveNamedFlow(document(), namedFlow);

    m_namedFlows.remove(namedFlow);
}

Document* NamedFlowCollection::document() const
{
    ScriptExecutionContext* context = ContextDestructionObserver::scriptExecutionContext();
    return toDocument(context);
}

PassRefPtr<DOMNamedFlowCollection> NamedFlowCollection::createCSSOMSnapshot()
{
    Vector<WebKitNamedFlow*> createdFlows;
    for (NamedFlowSet::iterator it = m_namedFlows.begin(); it != m_namedFlows.end(); ++it)
        if ((*it)->flowState() == WebKitNamedFlow::FlowStateCreated)
            createdFlows.append(*it);
    return DOMNamedFlowCollection::create(createdFlows);
}

// The HashFunctions object used by the HashSet to compare between NamedFlows.
// It is safe to set safeToCompareToEmptyOrDeleted because the HashSet will never contain null pointers or deleted values.
struct NamedFlowCollection::NamedFlowHashFunctions {
    static unsigned hash(WebKitNamedFlow* key) { return DefaultHash<String>::Hash::hash(key->name()); }
    static bool equal(WebKitNamedFlow* a, WebKitNamedFlow* b) { return a->name() == b->name(); }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

// The HashTranslator is used to lookup a NamedFlow in the set using a name.
struct NamedFlowCollection::NamedFlowHashTranslator {
    static unsigned hash(const String& key) { return DefaultHash<String>::Hash::hash(key); }
    static bool equal(WebKitNamedFlow* a, const String& b) { return a->name() == b; }
};

} // namespace WebCore
