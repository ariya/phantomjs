/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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
#include "WebGeolocationManagerProxy.h"

#include "WebContext.h"
#include "WebGeolocationManagerMessages.h"
#include "WebGeolocationManagerProxyMessages.h"

namespace WebKit {

const char* WebGeolocationManagerProxy::supplementName()
{
    return "WebGeolocationManagerProxy";
}

PassRefPtr<WebGeolocationManagerProxy> WebGeolocationManagerProxy::create(WebContext* context)
{
    return adoptRef(new WebGeolocationManagerProxy(context));
}

WebGeolocationManagerProxy::WebGeolocationManagerProxy(WebContext* context)
    : WebContextSupplement(context)
    , m_isUpdating(false)
{
    WebContextSupplement::context()->addMessageReceiver(Messages::WebGeolocationManagerProxy::messageReceiverName(), this);
}

WebGeolocationManagerProxy::~WebGeolocationManagerProxy()
{
}

void WebGeolocationManagerProxy::initializeProvider(const WKGeolocationProvider* provider)
{
    m_provider.initialize(provider);
}

// WebContextSupplement

void WebGeolocationManagerProxy::contextDestroyed()
{
    stopUpdating();
}

void WebGeolocationManagerProxy::processDidClose(WebProcessProxy*)
{
    stopUpdating();
}

void WebGeolocationManagerProxy::refWebContextSupplement()
{
    APIObject::ref();
}

void WebGeolocationManagerProxy::derefWebContextSupplement()
{
    APIObject::deref();
}

void WebGeolocationManagerProxy::providerDidChangePosition(WebGeolocationPosition* position)
{
    if (!context())
        return;

    context()->sendToAllProcesses(Messages::WebGeolocationManager::DidChangePosition(position->data()));
}

void WebGeolocationManagerProxy::providerDidFailToDeterminePosition(const String& errorMessage)
{
    if (!context())
        return;

    context()->sendToAllProcesses(Messages::WebGeolocationManager::DidFailToDeterminePosition(errorMessage));
}

void WebGeolocationManagerProxy::startUpdating()
{
    if (m_isUpdating)
        return;

    m_provider.startUpdating(this);
    m_isUpdating = true;
}

void WebGeolocationManagerProxy::stopUpdating()
{
    if (!m_isUpdating)
        return;

    m_provider.stopUpdating(this);
    m_isUpdating = false;
}

} // namespace WebKit
