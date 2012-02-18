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

#include "PlatformString.h"
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class Frame;
class InspectorArray;
class InspectorDOMStorageResource;
class InspectorFrontend;
class InspectorState;
class InstrumentingAgents;
class Storage;
class StorageArea;

typedef String ErrorString;

class InspectorDOMStorageAgent {
public:
    static PassOwnPtr<InspectorDOMStorageAgent> create(InstrumentingAgents* instrumentingAgents, InspectorState* state)
    {
        return adoptPtr(new InspectorDOMStorageAgent(instrumentingAgents, state));
    }
    ~InspectorDOMStorageAgent();

    void setFrontend(InspectorFrontend*);
    void clearFrontend();

    void clearResources();
    void restore();

    // Called from the front-end.
    void enable(ErrorString*);
    void disable(ErrorString*);
    void getDOMStorageEntries(ErrorString*, int storageId, RefPtr<InspectorArray>* entries);
    void setDOMStorageItem(ErrorString*, int storageId, const String& key, const String& value, bool* success);
    void removeDOMStorageItem(ErrorString*, int storageId, const String& key, bool* success);

    // Called from the injected script.
    int storageId(Storage*);

    // Called from InspectorInstrumentation
    void didUseDOMStorage(StorageArea*, bool isLocalStorage, Frame*);

private:
    InspectorDOMStorageAgent(InstrumentingAgents*, InspectorState*);

    InspectorDOMStorageResource* getDOMStorageResourceForId(int storageId);

    InstrumentingAgents* m_instrumentingAgents;
    InspectorState* m_inspectorState;
    typedef HashMap<int, RefPtr<InspectorDOMStorageResource> > DOMStorageResourcesMap;
    DOMStorageResourcesMap m_resources;
    InspectorFrontend* m_frontend;
    bool m_enabled;
};

} // namespace WebCore

#endif // !defined(InspectorDOMStorageAgent_h)
