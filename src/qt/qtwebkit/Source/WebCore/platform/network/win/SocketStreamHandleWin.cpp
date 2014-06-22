/*
 * Copyright (C) 2009 Brent Fulgham.  All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

#include "KURL.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "SocketStreamHandleClient.h"

namespace WebCore {

SocketStreamHandle::SocketStreamHandle(const KURL& url, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(url, client)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);
    notImplemented();
}

SocketStreamHandle::~SocketStreamHandle()
{
    LOG(Network, "SocketStreamHandle %p delete", this);
    setClient(0);
    notImplemented();
}

int SocketStreamHandle::platformSend(const char*, int)
{
    LOG(Network, "SocketStreamHandle %p platformSend", this);
    notImplemented();
    return 0;
}

void SocketStreamHandle::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);
    notImplemented();
}

void SocketStreamHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedCredential(const AuthenticationChallenge&, const Credential&)
{
    notImplemented();
}

void SocketStreamHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedCancellation(const AuthenticationChallenge&)
{
    notImplemented();
}

} // namespace WebCore
