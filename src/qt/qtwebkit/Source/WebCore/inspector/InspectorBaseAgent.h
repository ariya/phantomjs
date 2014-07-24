/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef InspectorBaseAgent_h
#define InspectorBaseAgent_h

#include "InspectorBackendDispatcher.h"
#include <wtf/Forward.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class InspectorFrontend;
class InspectorCompositeState;
class InspectorState;
class InstrumentingAgents;

class InspectorBaseAgentInterface {
public:
    InspectorBaseAgentInterface(const String&, InstrumentingAgents*, InspectorCompositeState*);
    virtual ~InspectorBaseAgentInterface();

    virtual void setFrontend(InspectorFrontend*) { }
    virtual void clearFrontend() { }
    virtual void restore() { }
    virtual void registerInDispatcher(InspectorBackendDispatcher*) = 0;
    virtual void discardAgent() { }

    String name() { return m_name; }

protected:
    InstrumentingAgents* m_instrumentingAgents;
    InspectorState* m_state;

private:
    String m_name;
};

class InspectorAgentRegistry {
public:
    void append(PassOwnPtr<InspectorBaseAgentInterface>);

    void setFrontend(InspectorFrontend*);
    void clearFrontend();
    void restore();
    void registerInDispatcher(InspectorBackendDispatcher*);
    void discardAgents();

private:
    Vector<OwnPtr<InspectorBaseAgentInterface> > m_agents;
};

template<typename T>
class InspectorBaseAgent : public InspectorBaseAgentInterface {
public:
    virtual ~InspectorBaseAgent() { }

    virtual void registerInDispatcher(InspectorBackendDispatcher* dispatcher)
    {
        dispatcher->registerAgent(static_cast<T*>(this));
    }

protected:
    InspectorBaseAgent(const String& name, InstrumentingAgents* instrumentingAgents, InspectorCompositeState* inspectorState)
        : InspectorBaseAgentInterface(name, instrumentingAgents, inspectorState)
    {
    }
};

} // namespace WebCore

#endif // !defined(InspectorBaseAgent_h)
