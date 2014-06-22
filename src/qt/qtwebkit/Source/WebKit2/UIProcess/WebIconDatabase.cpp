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
#include "WebIconDatabase.h"

#include "DataReference.h"
#include "Logging.h"
#include "WebContext.h"
#include "WebIconDatabaseMessages.h"
#include "WebIconDatabaseProxyMessages.h"
#include <WebCore/FileSystem.h>
#include <WebCore/IconDatabase.h>
#include <WebCore/IconDatabaseBase.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebIconDatabase> WebIconDatabase::create(WebContext* context)
{
    return adoptRef(new WebIconDatabase(context));
}

WebIconDatabase::~WebIconDatabase()
{
}

WebIconDatabase::WebIconDatabase(WebContext* context)
    : m_webContext(context)
    , m_urlImportCompleted(false)
    , m_databaseCleanupDisabled(false)
{
    m_webContext->addMessageReceiver(Messages::WebIconDatabase::messageReceiverName(), this);
}

void WebIconDatabase::invalidate()
{
    setGlobalIconDatabase(0);
}

void WebIconDatabase::setDatabasePath(const String& path)
{
    if (isOpen()) {
        LOG_ERROR("Icon database already has a path and is already open. We don't currently support changing its path and reopening.");
        return;
    }

    m_iconDatabaseImpl =  IconDatabase::create();
    m_iconDatabaseImpl->setClient(this);
    IconDatabase::delayDatabaseCleanup();
    m_databaseCleanupDisabled = true;
    m_iconDatabaseImpl->setEnabled(true);
    if (!m_iconDatabaseImpl->open(directoryName(path), pathGetFileName(path))) {
        LOG_ERROR("Unable to open WebKit2 icon database on disk");
        m_iconDatabaseImpl.clear();
        setGlobalIconDatabase(0);
        IconDatabase::allowDatabaseCleanup();
        m_databaseCleanupDisabled = false;
    }
    setGlobalIconDatabase(m_iconDatabaseImpl.get());
}

void WebIconDatabase::enableDatabaseCleanup()
{
    if (!m_iconDatabaseImpl) {
        LOG_ERROR("Cannot enabled Icon Database cleanup - it hasn't been opened yet.");
        return;
    }

    if (!m_databaseCleanupDisabled) {
        LOG_ERROR("Attempt to enable database cleanup, but it's already enabled.");
        ASSERT_NOT_REACHED();
        return;
    }
    
    IconDatabase::allowDatabaseCleanup();
    m_databaseCleanupDisabled = false;
}

void WebIconDatabase::retainIconForPageURL(const String& pageURL)
{
    if (m_iconDatabaseImpl)
        m_iconDatabaseImpl->retainIconForPageURL(pageURL);
}

void WebIconDatabase::releaseIconForPageURL(const String& pageURL)
{
    if (m_iconDatabaseImpl)
        m_iconDatabaseImpl->releaseIconForPageURL(pageURL);
}

void WebIconDatabase::setIconURLForPageURL(const String& iconURL, const String& pageURL)
{
    LOG(IconDatabase, "WK2 UIProcess setting icon URL %s for page URL %s", iconURL.ascii().data(), pageURL.ascii().data());
    if (m_iconDatabaseImpl)
        m_iconDatabaseImpl->setIconURLForPageURL(iconURL, pageURL);
}

void WebIconDatabase::setIconDataForIconURL(const CoreIPC::DataReference& iconData, const String& iconURL)
{
    LOG(IconDatabase, "WK2 UIProcess setting icon data (%i bytes) for page URL %s", (int)iconData.size(), iconURL.ascii().data());
    if (!m_iconDatabaseImpl)
        return;

    RefPtr<SharedBuffer> buffer = SharedBuffer::create(iconData.data(), iconData.size());
    m_iconDatabaseImpl->setIconDataForIconURL(buffer.release(), iconURL);
}

void WebIconDatabase::synchronousIconDataForPageURL(const String&, CoreIPC::DataReference& iconData)
{
    iconData = CoreIPC::DataReference();
}

void WebIconDatabase::synchronousIconURLForPageURL(const String& pageURL, String& iconURL)
{
    if (!m_iconDatabaseImpl) {
        iconURL = String();
        return;
    }

    iconURL = m_iconDatabaseImpl->synchronousIconURLForPageURL(pageURL);
}

void WebIconDatabase::synchronousIconDataKnownForIconURL(const String&, bool& iconDataKnown) const
{
    iconDataKnown = false;
}

void WebIconDatabase::synchronousLoadDecisionForIconURL(const String&, int& loadDecision) const
{
    loadDecision = static_cast<int>(IconLoadNo);
}

void WebIconDatabase::getLoadDecisionForIconURL(const String& iconURL, uint64_t callbackID)
{
    LOG(IconDatabase, "WK2 UIProcess getting load decision for icon URL %s with callback ID %lli", iconURL.ascii().data(), static_cast<long long>(callbackID));

    if (!m_webContext)
        return;

    if (!m_iconDatabaseImpl || !m_iconDatabaseImpl->isOpen() || iconURL.isEmpty()) {
        // FIXME (Multi-WebProcess): <rdar://problem/12240223> We need to know which connection to send this message to.
        m_webContext->sendToAllProcesses(Messages::WebIconDatabaseProxy::ReceivedIconLoadDecision(static_cast<int>(IconLoadNo), callbackID));
        return;
    }
    
    // If the decision hasn't been read from disk yet, set this url and callback ID aside to be notifed later
    IconLoadDecision decision = m_iconDatabaseImpl->synchronousLoadDecisionForIconURL(iconURL, 0);
    if (decision == IconLoadUnknown) {
        // We should never get an unknown load decision after the URL import has completed.
        ASSERT(!m_urlImportCompleted);
        
        m_pendingLoadDecisionURLMap.set(callbackID, iconURL);
        return;    
    }

    // FIXME (Multi-WebProcess): <rdar://problem/12240223> We need to know which connection to send this message to.
    m_webContext->sendToAllProcesses(Messages::WebIconDatabaseProxy::ReceivedIconLoadDecision((int)decision, callbackID));
}

void WebIconDatabase::didReceiveIconForPageURL(const String& pageURL)
{
    notifyIconDataReadyForPageURL(pageURL);
}

Image* WebIconDatabase::imageForPageURL(const String& pageURL, const WebCore::IntSize& iconSize)
{
    if (!m_webContext || !m_iconDatabaseImpl || !m_iconDatabaseImpl->isOpen() || pageURL.isEmpty())
        return 0;    

    // The WebCore IconDatabase ignores the passed in size parameter.
    // If that changes we'll need to rethink how this API is exposed.
    return m_iconDatabaseImpl->synchronousIconForPageURL(pageURL, iconSize);
}

WebCore::NativeImagePtr WebIconDatabase::nativeImageForPageURL(const String& pageURL, const WebCore::IntSize& iconSize)
{
    if (!m_webContext || !m_iconDatabaseImpl || !m_iconDatabaseImpl->isOpen() || pageURL.isEmpty())
        return 0;

    return m_iconDatabaseImpl->synchronousNativeIconForPageURL(pageURL, iconSize);
}

bool WebIconDatabase::isOpen()
{
    return m_iconDatabaseImpl && m_iconDatabaseImpl->isOpen();
}

bool WebIconDatabase::isUrlImportCompleted()
{
    return m_urlImportCompleted;
}

void WebIconDatabase::removeAllIcons()
{
    m_iconDatabaseImpl->removeAllIcons();   
}

void WebIconDatabase::checkIntegrityBeforeOpening()
{
    IconDatabase::checkIntegrityBeforeOpening();
}

void WebIconDatabase::close()
{
    if (m_iconDatabaseImpl)
        m_iconDatabaseImpl->close();
}

void WebIconDatabase::initializeIconDatabaseClient(const WKIconDatabaseClient* client)
{
    m_iconDatabaseClient.initialize(client);
}

// WebCore::IconDatabaseClient

void WebIconDatabase::didImportIconURLForPageURL(const String& pageURL)
{
    didChangeIconForPageURL(pageURL);
}

void WebIconDatabase::didImportIconDataForPageURL(const String& pageURL)
{
    notifyIconDataReadyForPageURL(pageURL);
}

void WebIconDatabase::didChangeIconForPageURL(const String& pageURL)
{
    m_iconDatabaseClient.didChangeIconForPageURL(this, WebURL::create(pageURL).get());
}

void WebIconDatabase::didRemoveAllIcons()
{
    m_iconDatabaseClient.didRemoveAllIcons(this);
}

void WebIconDatabase::didFinishURLImport()
{
    if (!m_webContext)
        return;
    
    ASSERT(!m_urlImportCompleted);

    LOG(IconDatabase, "WK2 UIProcess URL import complete, notifying all %i pending page URL load decisions", m_pendingLoadDecisionURLMap.size());

    HashMap<uint64_t, String>::iterator i = m_pendingLoadDecisionURLMap.begin();
    HashMap<uint64_t, String>::iterator end = m_pendingLoadDecisionURLMap.end();
    
    for (; i != end; ++i) {
        LOG(IconDatabase, "WK2 UIProcess performing delayed callback on callback ID %i for page url %s", (int)i->key, i->value.ascii().data());
        IconLoadDecision decision = m_iconDatabaseImpl->synchronousLoadDecisionForIconURL(i->value, 0);

        // Decisions should never be unknown after the inital import is complete
        ASSERT(decision != IconLoadUnknown);

        // FIXME (Multi-WebProcess): <rdar://problem/12240223> We need to know which connection to send this message to.
        m_webContext->sendToAllProcesses(Messages::WebIconDatabaseProxy::ReceivedIconLoadDecision(static_cast<int>(decision), i->key));
    }
    
    m_pendingLoadDecisionURLMap.clear();
    
    m_urlImportCompleted = true;
}

void WebIconDatabase::notifyIconDataReadyForPageURL(const String& pageURL)
{
    m_iconDatabaseClient.iconDataReadyForPageURL(this, WebURL::create(pageURL).get());
    didChangeIconForPageURL(pageURL);
}

} // namespace WebKit
