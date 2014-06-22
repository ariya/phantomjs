/*
 * Copyright (C) 2012 Samsung Electronics. All Rights Reserved.
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

#ifndef NetworkInfoConnection_h
#define NetworkInfoConnection_h

#if ENABLE(NETWORK_INFO)
#include "ActiveDOMObject.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "Navigator.h"
#include "NetworkInfo.h"
#include "NetworkInfoController.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class NetworkInfoController;

class NetworkInfoConnection : public RefCounted<NetworkInfoConnection>, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<NetworkInfoConnection> create(Navigator*);

    ~NetworkInfoConnection();

    double bandwidth() const;
    bool metered() const;
    
    void didChangeNetworkInformation(PassRefPtr<Event>, PassRefPtr<NetworkInfo>);
    void networkInfoControllerDestroyed() { m_controller = 0; }

    // EventTarget implementation.
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const { return ActiveDOMObject::scriptExecutionContext(); }

    using RefCounted<NetworkInfoConnection>::ref;
    using RefCounted<NetworkInfoConnection>::deref;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitnetworkinfochange);
  
    // ActiveDOMObject implementation.
    virtual bool canSuspend() const { return true; }
    virtual void suspend(ReasonForSuspension);
    virtual void resume();
    virtual void stop();

private:
    explicit NetworkInfoConnection(Navigator*);

    // EventTarget implementation.
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }

    // EventTarget implementation.
    EventTargetData m_eventTargetData;

    NetworkInfoController* m_controller;
    RefPtr<NetworkInfo> m_networkInfo; 
};

} // namespace WebCore

#endif
#endif // NetworkInfoConnection_h
