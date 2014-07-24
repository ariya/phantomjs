/*
 * Copyright (C) 2012 Samsung Electronics. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef NetworkInfoController_h
#define NetworkInfoController_h

#if ENABLE(NETWORK_INFO)
#include "NetworkInfoClient.h"
#include "NetworkInfoConnection.h"
#include "Page.h"

namespace WebCore {

class NetworkInfo;
class NetworkInfoConnection;

class NetworkInfoController : public Supplement<Page> {
public:
    ~NetworkInfoController();

    NetworkInfoClient* client() { return m_client; }

    static PassOwnPtr<NetworkInfoController> create(Page*, NetworkInfoClient*);

    static const char* supplementName();
    static NetworkInfoController* from(Page* page) { return static_cast<NetworkInfoController*>(Supplement<Page>::from(page, supplementName())); }
   
    void addListener(NetworkInfoConnection*);
    void removeListener(NetworkInfoConnection*);

    void didChangeNetworkInformation(const AtomicString& eventType, PassRefPtr<NetworkInfo>);

private:
    NetworkInfoController(Page*, NetworkInfoClient*);

    typedef Vector<NetworkInfoConnection*> NetworkInfoListenerList;

    Page* m_page;
    NetworkInfoClient* m_client;
    NetworkInfoListenerList m_listeners;
};

}

#endif // ENABLE(NETWORK_INFO)
#endif // NetworkInfoController_h
