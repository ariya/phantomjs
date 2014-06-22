/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
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

#ifndef WebKitNamedFlow_h
#define WebKitNamedFlow_h

#include "EventTarget.h"

#include <wtf/ListHashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class Document;
class NamedFlowCollection;
class Node;
class NodeList;
class RenderNamedFlowThread;
class ScriptExecutionContext;

class WebKitNamedFlow : public RefCounted<WebKitNamedFlow>, public EventTarget {
public:
    static PassRefPtr<WebKitNamedFlow> create(PassRefPtr<NamedFlowCollection> manager, const AtomicString& flowThreadName);

    ~WebKitNamedFlow();

    const AtomicString& name() const;
    bool overset() const;
    int firstEmptyRegionIndex() const;
    PassRefPtr<NodeList> getRegionsByContent(Node*);
    PassRefPtr<NodeList> getRegions();
    PassRefPtr<NodeList> getContent();

    using RefCounted<WebKitNamedFlow>::ref;
    using RefCounted<WebKitNamedFlow>::deref;

    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;

    // This function is called from the JS binding code to determine if the NamedFlow object is reachable or not.
    // If the object has listeners, the object should only be discarded if the parent Document is not reachable.
    Node* ownerNode() const;

    void setRenderer(RenderNamedFlowThread* parentFlowThread);

    enum FlowState {
        FlowStateCreated,
        FlowStateNull
    };

    FlowState flowState() const { return m_parentFlowThread ? FlowStateCreated : FlowStateNull; }

    void dispatchRegionLayoutUpdateEvent();
    void dispatchRegionOversetChangeEvent();

private:
    WebKitNamedFlow(PassRefPtr<NamedFlowCollection>, const AtomicString&);

    // EventTarget implementation.
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }

    virtual EventTargetData* eventTargetData() OVERRIDE;
    virtual EventTargetData* ensureEventTargetData() OVERRIDE;

    // The name of the flow thread as specified in CSS.
    AtomicString m_flowThreadName;

    RefPtr<NamedFlowCollection> m_flowManager;
    RenderNamedFlowThread* m_parentFlowThread;

    EventTargetData m_eventTargetData;
};

}

#endif
