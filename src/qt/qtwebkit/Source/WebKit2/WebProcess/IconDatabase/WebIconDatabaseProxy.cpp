/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "WebIconDatabaseProxy.h"

#include "DataReference.h"
#include "WebIconDatabaseMessages.h"
#include "WebIconDatabaseProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/SharedBuffer.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

WebIconDatabaseProxy::~WebIconDatabaseProxy()
{
}

WebIconDatabaseProxy::WebIconDatabaseProxy(WebProcess* process)
    : m_isEnabled(false)
    , m_process(process)
{
    m_process->addMessageReceiver(Messages::WebIconDatabaseProxy::messageReceiverName(), this);
}

bool WebIconDatabaseProxy::isEnabled() const
{
    return m_isEnabled;
}

void WebIconDatabaseProxy::setEnabled(bool enabled)
{
    if (enabled == m_isEnabled)
        return;
    
    m_isEnabled = enabled;
    setGlobalIconDatabase(enabled ? this : 0);
}

void WebIconDatabaseProxy::retainIconForPageURL(const String& pageURL)
{
    m_process->parentProcessConnection()->send(Messages::WebIconDatabase::RetainIconForPageURL(pageURL), 0);
}

void WebIconDatabaseProxy::releaseIconForPageURL(const String& pageURL)
{
    m_process->parentProcessConnection()->send(Messages::WebIconDatabase::ReleaseIconForPageURL(pageURL), 0);
}

Image* WebIconDatabaseProxy::synchronousIconForPageURL(const String& pageURL, const IntSize& /*size*/)
{
    CoreIPC::DataReference result;
    if (!m_process->parentProcessConnection()->sendSync(Messages::WebIconDatabase::SynchronousIconDataForPageURL(pageURL), Messages::WebIconDatabase::SynchronousIconDataForPageURL::Reply(result), 0))
        return 0;
    
    // FIXME: Return Image created with the above data.
    return 0;
}


String WebIconDatabaseProxy::synchronousIconURLForPageURL(const String& /*pageURL*/)
{
    // FIXME: This needs to ask the UI process for the iconURL, but it can't do so synchronously because it will slow down page loading.
    // The parts in WebCore that need this data will have to be changed to work asycnchronously.
    return String();
}

bool WebIconDatabaseProxy::synchronousIconDataKnownForIconURL(const String& /*iconURL*/)
{
    // FIXME: This needs to ask the UI process for the iconURL, but it can't do so synchronously because it will slow down page loading.
    // The parts in WebCore that need this data will have to be changed to work asycnchronously.
    return false;
}

IconLoadDecision WebIconDatabaseProxy::synchronousLoadDecisionForIconURL(const String& /*iconURL*/, DocumentLoader*)
{
    // FIXME: This needs to ask the UI process for the iconURL, but it can't do so synchronously because it will slow down page loading.
    // The parts in WebCore that need this data will have to be changed to work asycnchronously.
    return IconLoadNo;
}

bool WebIconDatabaseProxy::supportsAsynchronousMode()
{
    return true;
}

void WebIconDatabaseProxy::loadDecisionForIconURL(const String& iconURL, PassRefPtr<WebCore::IconLoadDecisionCallback> callback)
{
    uint64_t id = callback->callbackID();
    m_iconLoadDecisionCallbacks.add(id, callback);
    
    m_process->parentProcessConnection()->send(Messages::WebIconDatabase::GetLoadDecisionForIconURL(iconURL, id), 0);
}

void WebIconDatabaseProxy::receivedIconLoadDecision(int decision, uint64_t callbackID)
{
    RefPtr<WebCore::IconLoadDecisionCallback> callback = m_iconLoadDecisionCallbacks.take(callbackID);
    if (callback)
        callback->performCallback(static_cast<WebCore::IconLoadDecision>(decision));
}

void WebIconDatabaseProxy::iconDataForIconURL(const String& /*iconURL*/, PassRefPtr<WebCore::IconDataCallback>)
{
}

void WebIconDatabaseProxy::setIconURLForPageURL(const String& iconURL, const String& pageURL)
{
    m_process->parentProcessConnection()->send(Messages::WebIconDatabase::SetIconURLForPageURL(iconURL, pageURL), 0);
}

void WebIconDatabaseProxy::setIconDataForIconURL(PassRefPtr<SharedBuffer> iconData, const String& iconURL)
{
    CoreIPC::DataReference data(reinterpret_cast<const uint8_t*>(iconData ? iconData->data() : 0), iconData ? iconData->size() : 0);
    m_process->parentProcessConnection()->send(Messages::WebIconDatabase::SetIconDataForIconURL(data, iconURL), 0);
}

void WebIconDatabaseProxy::urlImportFinished()
{
}

} // namespace WebKit
