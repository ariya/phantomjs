/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "WebNetworkInfoClient.h"

#if ENABLE(NETWORK_INFO)

#include "WebNetworkInfoManager.h"
#include "WebPage.h"
#include "WebProcess.h"

namespace WebKit {

WebNetworkInfoClient::~WebNetworkInfoClient()
{
}

double WebNetworkInfoClient::bandwidth() const
{
    return WebProcess::shared().supplement<WebNetworkInfoManager>()->bandwidth(m_page);
}

bool WebNetworkInfoClient::metered() const
{
    return WebProcess::shared().supplement<WebNetworkInfoManager>()->metered(m_page);
}

void WebNetworkInfoClient::startUpdating()
{
    WebProcess::shared().supplement<WebNetworkInfoManager>()->registerWebPage(m_page);
}

void WebNetworkInfoClient::stopUpdating()
{
    WebProcess::shared().supplement<WebNetworkInfoManager>()->unregisterWebPage(m_page);
}

void WebNetworkInfoClient::networkInfoControllerDestroyed()
{
    WebProcess::shared().supplement<WebNetworkInfoManager>()->unregisterWebPage(m_page);
    delete this;
}

} // namespace WebKit

#endif // ENABLE(NETWORK_INFO)
