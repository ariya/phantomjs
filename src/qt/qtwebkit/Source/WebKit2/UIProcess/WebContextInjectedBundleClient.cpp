/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebContextInjectedBundleClient.h"

#include "WKAPICast.h"
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

void WebContextInjectedBundleClient::didReceiveMessageFromInjectedBundle(WebContext* context, const String& messageName, APIObject* messageBody)
{
    if (!m_client.didReceiveMessageFromInjectedBundle)
        return;

    m_client.didReceiveMessageFromInjectedBundle(toAPI(context), toAPI(messageName.impl()), toAPI(messageBody), m_client.clientInfo);
}

void WebContextInjectedBundleClient::didReceiveSynchronousMessageFromInjectedBundle(WebContext* context, const String& messageName, APIObject* messageBody, RefPtr<APIObject>& returnData)
{
    if (!m_client.didReceiveSynchronousMessageFromInjectedBundle)
        return;

    WKTypeRef returnDataRef = 0;
    m_client.didReceiveSynchronousMessageFromInjectedBundle(toAPI(context), toAPI(messageName.impl()), toAPI(messageBody), &returnDataRef, m_client.clientInfo);
    returnData = adoptRef(toImpl(returnDataRef));
}

PassRefPtr<APIObject> WebContextInjectedBundleClient::getInjectedBundleInitializationUserData(WebContext* context)
{
    if (!m_client.getInjectedBundleInitializationUserData)
        return 0;

    return toImpl(m_client.getInjectedBundleInitializationUserData(toAPI(context), m_client.clientInfo));
}

} // namespace WebKit
