/*
 * Copyright (C) 2009, 2011, 2012 Google Inc.  All rights reserved.
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
#include "SocketStreamHandleBase.h"

#include "SocketStreamHandle.h"
#include "SocketStreamHandleClient.h"

namespace WebCore {

const unsigned int bufferSize = 100 * 1024 * 1024;

SocketStreamHandleBase::SocketStreamHandleBase(const KURL& url, SocketStreamHandleClient* client)
    : m_url(url)
    , m_client(client)
    , m_state(Connecting)
{
}

SocketStreamHandleBase::SocketStreamState SocketStreamHandleBase::state() const
{
    return m_state;
}

bool SocketStreamHandleBase::send(const char* data, int length)
{
    if (m_state == Connecting || m_state == Closing)
        return false;
    if (!m_buffer.isEmpty()) {
        if (m_buffer.size() + length > bufferSize) {
            // FIXME: report error to indicate that buffer has no more space.
            return false;
        }
        m_buffer.append(data, length);
        if (m_client)
            m_client->didUpdateBufferedAmount(static_cast<SocketStreamHandle*>(this), bufferedAmount());
        return true;
    }
    int bytesWritten = 0;
    if (m_state == Open)
        bytesWritten = platformSend(data, length);
    if (bytesWritten < 0)
        return false;
    if (m_buffer.size() + length - bytesWritten > bufferSize) {
        // FIXME: report error to indicate that buffer has no more space.
        return false;
    }
    if (bytesWritten < length) {
        m_buffer.append(data + bytesWritten, length - bytesWritten);
        if (m_client)
            m_client->didUpdateBufferedAmount(static_cast<SocketStreamHandle*>(this), bufferedAmount());
    }
    return true;
}

void SocketStreamHandleBase::close()
{
    if (m_state == Closed)
        return;
    m_state = Closing;
    if (!m_buffer.isEmpty())
        return;
    disconnect();
}

void SocketStreamHandleBase::disconnect()
{
    RefPtr<SocketStreamHandle> protect(static_cast<SocketStreamHandle*>(this)); // platformClose calls the client, which may make the handle get deallocated immediately.

    platformClose();
    m_state = Closed;
}

void SocketStreamHandleBase::setClient(SocketStreamHandleClient* client)
{
    ASSERT(!client || (!m_client && m_state == Connecting));
    m_client = client;
}

bool SocketStreamHandleBase::sendPendingData()
{
    if (m_state != Open && m_state != Closing)
        return false;
    if (m_buffer.isEmpty()) {
        if (m_state == Open)
            return false;
        if (m_state == Closing) {
            disconnect();
            return false;
        }
    }
    bool pending;
    do {
        int bytesWritten = platformSend(m_buffer.firstBlockData(), m_buffer.firstBlockSize());
        pending = bytesWritten != static_cast<int>(m_buffer.firstBlockSize());
        if (bytesWritten <= 0)
            return false;
        ASSERT(m_buffer.size() - bytesWritten <= bufferSize);
        m_buffer.consume(bytesWritten);
    } while (!pending && !m_buffer.isEmpty());
    if (m_client)
        m_client->didUpdateBufferedAmount(static_cast<SocketStreamHandle*>(this), bufferedAmount());
    return true;
}

}  // namespace WebCore
