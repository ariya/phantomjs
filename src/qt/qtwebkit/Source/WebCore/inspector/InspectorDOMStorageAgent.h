/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorDOMStorageAgent_h
#define InspectorDOMStorageAgent_h

#include "InspectorBaseAgent.h"
#include "StorageArea.h"
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Frame;
class InspectorArray;
class InspectorFrontend;
class InspectorPageAgent;
class InspectorState;
class InstrumentingAgents;
class Page;
class SecurityOrigin;
class Storage;
class StorageArea;

typedef String ErrorString;

class InspectorDOMStorageAgent : public InspectorBaseAgent<InspectorDOMStorageAgent>, public InspectorBackendDispatcher::DOMStorageCommandHandler {
public:
    static PassOwnPtr<InspectorDOMStorageAgent> create(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorCompositeState* state)
    {
        return adoptPtr(new InspectorDOMStorageAgent(instrumentingAgents, pageAgent, state));
    }
    ~InspectorDOMStorageAgent();

    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();

    // Called from the front-end.
    virtual void enable(ErrorString*);
    virtual void disable(ErrorString*);
    virtual void getDOMStorageItems(ErrorString*, const RefPtr<InspectorObject>& storageId, RefPtr<TypeBuilder::Array<TypeBuilder::Array<String> > >& items);
    virtual void setDOMStorageItem(ErrorString*, const RefPtr<InspectorObject>& storageId, const String& key, const String& value);
    virtual void removeDOMStorageItem(ErrorString*, const RefPtr<InspectorObject>& storageId, const String& key);

    // Called from the injected script.
    String storageId(Storage*);
    PassRefPtr<TypeBuilder::DOMStorage::StorageId> storageId(SecurityOrigin*, bool isLocalStorage);

    // Called from InspectorInstrumentation
    void didDispatchDOMStorageEvent(const String& key, const String& oldValue, const String& newValue, StorageType, SecurityOrigin*, Page*);

private:

    InspectorDOMStorageAgent(InstrumentingAgents*, InspectorPageAgent*, InspectorCompositeState*);

    bool isEnabled() const;
    PassRefPtr<StorageArea> findStorageArea(ErrorString*, const RefPtr<InspectorObject>&, Frame*&);

    InspectorPageAgent* m_pageAgent;
    InspectorFrontend* m_frontend;
};

} // namespace WebCore

#endif // !defined(InspectorDOMStorageAgent_h)
