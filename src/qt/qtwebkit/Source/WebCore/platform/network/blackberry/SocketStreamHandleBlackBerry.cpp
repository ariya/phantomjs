/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (C) 2010 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SocketStreamHandle.h"

#include "Chrome.h"
#include "FrameLoaderClientBlackBerry.h"
#include "KURL.h"
#include "Logging.h"
#include "Page.h"
#include "PageClientBlackBerry.h"
#include "PageGroup.h"
#include "SocketStreamError.h"
#include "SocketStreamHandleClient.h"

#include <network/NetworkStreamFactory.h>
#include <wtf/text/CString.h>

namespace WebCore {

SocketStreamHandle::SocketStreamHandle(const String& groupName, const KURL& url, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(url, client)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);

    // Find a playerId to pass to the platform client. It can be from any page
    // in the PageGroup, since they all share the same network profile and
    // resources.
    PageGroup* pageGroup = PageGroup::pageGroup(groupName);
    ASSERT(pageGroup && !pageGroup->pages().isEmpty());
    Page* page = *(pageGroup->pages().begin());
    ASSERT(page && page->mainFrame());
    int playerId = static_cast<FrameLoaderClientBlackBerry*>(page->mainFrame()->loader()->client())->playerId();

    // Create a platform socket stream
    BlackBerry::Platform::NetworkStreamFactory* factory = page->chrome().platformPageClient()->networkStreamFactory();
    ASSERT(factory);

    // Open the socket
    BlackBerry::Platform::NetworkRequest request;
    STATIC_LOCAL_STRING(s_connect, "CONNECT");
    request.setRequestUrl(url.string(), s_connect);
    m_socketStream = adoptPtr(factory->createNetworkStream(request, playerId));
    ASSERT(m_socketStream);

    m_socketStream->setListener(this);
    m_socketStream->streamOpen();
}

SocketStreamHandle::~SocketStreamHandle()
{
    LOG(Network, "SocketStreamHandle %p delete", this);
    setClient(0);
}

int SocketStreamHandle::platformSend(const char* buf, int length)
{
    LOG(Network, "SocketStreamHandle %p platformSend", this);
    ASSERT(m_socketStream);
    return m_socketStream->streamSendData(buf, length);
}

void SocketStreamHandle::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);
    ASSERT(m_socketStream);
    m_socketStream->streamClose();
}

// FilterStream interface

void SocketStreamHandle::notifyStatusReceived(int status, const BlackBerry::Platform::String&)
{
    ASSERT(m_client);

    // The client can close the handle, potentially removing the last reference.
    RefPtr<SocketStreamHandle> protect(this);
    m_status = status;
    if (FilterStream::StatusSuccess != status)
        m_client->didFailSocketStream(this, SocketStreamError(status, message));
    else {
        m_state = Open;
        m_client->didOpenSocketStream(this);
    }
}

void SocketStreamHandle::notifyDataReceived(BlackBerry::Platform::NetworkBuffer* buffer)
{
    ASSERT(m_client);

    // The client can close the handle, potentially removing the last reference.
    RefPtr<SocketStreamHandle> protect(this);
    m_client->didReceiveSocketStreamData(this, BlackBerry::Platform::networkBufferData(buffer), BlackBerry::Platform::networkBufferDataLength(buffer));
}

void SocketStreamHandle::notifyReadyToSendData()
{
    sendPendingData();
}

void SocketStreamHandle::notifyClose(int status)
{
    ASSERT(m_client);

    // The client can close the handle, potentially removing the last reference.
    RefPtr<SocketStreamHandle> protect(this);

    if (status < 0 || (400 <= status && status < 600))
        m_status = status;

    if (FilterStream::StatusSuccess != status)
        m_client->didFailSocketStream(this, SocketStreamError(status));

    m_client->didCloseSocketStream(this);
}

} // namespace WebCore
