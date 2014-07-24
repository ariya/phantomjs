/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "NetworkBlobRegistry.h"

#if ENABLE(BLOB) && ENABLE(NETWORK_PROCESS)

#include "SandboxExtension.h"
#include <WebCore/BlobRegistryImpl.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace WebKit {

NetworkBlobRegistry& NetworkBlobRegistry::shared()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(NetworkBlobRegistry, registry, ());
    return registry;
}

NetworkBlobRegistry::NetworkBlobRegistry()
{
}

void NetworkBlobRegistry::registerBlobURL(NetworkConnectionToWebProcess* connection, const KURL& url, PassOwnPtr<BlobData> data, const Vector<RefPtr<SandboxExtension>>& newSandboxExtensions)
{
    ASSERT(!m_sandboxExtensions.contains(url.string()));

    // Combine new extensions for File items and existing extensions for inner Blob items.
    Vector<RefPtr<SandboxExtension>> sandboxExtensions = newSandboxExtensions;
    const BlobDataItemList& items = data->items();
    for (size_t i = 0, count = items.size(); i < count; ++i) {
        if (items[i].type == BlobDataItem::Blob)
            sandboxExtensions.appendVector(m_sandboxExtensions.get(items[i].url.string()));
    }

    blobRegistry().registerBlobURL(url, data);

    if (!sandboxExtensions.isEmpty())
        m_sandboxExtensions.add(url.string(), sandboxExtensions);

    ASSERT(!m_blobsForConnection.get(connection).contains(url));
    BlobForConnectionMap::iterator mapIterator = m_blobsForConnection.find(connection);
    if (mapIterator == m_blobsForConnection.end())
        mapIterator = m_blobsForConnection.add(connection, HashSet<KURL>()).iterator;
    mapIterator->value.add(url);
}

void NetworkBlobRegistry::registerBlobURL(NetworkConnectionToWebProcess* connection, const WebCore::KURL& url, const WebCore::KURL& srcURL)
{
    blobRegistry().registerBlobURL(url, srcURL);
    SandboxExtensionMap::iterator iter = m_sandboxExtensions.find(srcURL.string());
    if (iter != m_sandboxExtensions.end())
        m_sandboxExtensions.add(url.string(), iter->value);

    ASSERT(m_blobsForConnection.contains(connection));
    ASSERT(m_blobsForConnection.find(connection)->value.contains(srcURL));
    m_blobsForConnection.find(connection)->value.add(url);
}

void NetworkBlobRegistry::unregisterBlobURL(NetworkConnectionToWebProcess* connection, const WebCore::KURL& url)
{
    blobRegistry().unregisterBlobURL(url);
    m_sandboxExtensions.remove(url.string());

    ASSERT(m_blobsForConnection.contains(connection));
    ASSERT(m_blobsForConnection.find(connection)->value.contains(url));
    m_blobsForConnection.find(connection)->value.remove(url);
}

void NetworkBlobRegistry::connectionToWebProcessDidClose(NetworkConnectionToWebProcess* connection)
{
    if (!m_blobsForConnection.contains(connection))
        return;

    HashSet<KURL>& blobsForConnection = m_blobsForConnection.find(connection)->value;
    for (HashSet<KURL>::iterator iter = blobsForConnection.begin(), end = blobsForConnection.end(); iter != end; ++iter) {
        blobRegistry().unregisterBlobURL(*iter);
        m_sandboxExtensions.remove(*iter);
    }

    m_blobsForConnection.remove(connection);
}

const Vector<RefPtr<SandboxExtension>> NetworkBlobRegistry::sandboxExtensions(const WebCore::KURL& url)
{
    return m_sandboxExtensions.get(url.string());
}

}

#endif
