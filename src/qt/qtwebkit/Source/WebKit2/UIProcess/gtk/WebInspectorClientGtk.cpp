/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebInspectorClientGtk.h"

#include "WKAPICast.h"
#include "WKSharedAPICast.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

bool WebInspectorClientGtk::openWindow(WebInspectorProxy* inspector)
{
    if (!m_client.openWindow)
        return false;
    return m_client.openWindow(toAPI(inspector), m_client.clientInfo);
}

void WebInspectorClientGtk::didClose(WebInspectorProxy* inspector)
{
    if (!m_client.didClose)
        return;
    m_client.didClose(toAPI(inspector), m_client.clientInfo);
}

bool WebInspectorClientGtk::bringToFront(WebInspectorProxy* inspector)
{
    if (!m_client.bringToFront)
        return false;
    return m_client.bringToFront(toAPI(inspector), m_client.clientInfo);
}

void WebInspectorClientGtk::inspectedURLChanged(WebInspectorProxy* inspector, const String& url)
{
    if (!m_client.inspectedURLChanged)
        return;
    m_client.inspectedURLChanged(toAPI(inspector), toAPI(url.impl()), m_client.clientInfo);
}

bool WebInspectorClientGtk::attach(WebInspectorProxy* inspector)
{
    if (!m_client.attach)
        return false;
    return m_client.attach(toAPI(inspector), m_client.clientInfo);
}

bool WebInspectorClientGtk::detach(WebInspectorProxy* inspector)
{
    if (!m_client.detach)
        return false;
    return m_client.detach(toAPI(inspector), m_client.clientInfo);
}

void WebInspectorClientGtk::didChangeAttachedHeight(WebInspectorProxy* inspector, unsigned height)
{
    if (!m_client.didChangeAttachedHeight)
        return;
    m_client.didChangeAttachedHeight(toAPI(inspector), height, m_client.clientInfo);
}

} // namespace WebKit
