/*
 * Copyright (C) 2009, 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2012 Samsung Electronics Ltd. All Rights Reserved.
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
#include "SocketStreamError.h"
#include "SocketStreamHandleClient.h"

#include <gio/gio.h>
#include <glib.h>

#include <wtf/NotFound.h>
#include <wtf/Vector.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#define READ_BUFFER_SIZE 1024

namespace WebCore {

// These functions immediately call the similarly named SocketStreamHandle methods.
static void connectedCallback(GSocketClient*, GAsyncResult*, void*);
static void readReadyCallback(GInputStream*, GAsyncResult*, void*);
static gboolean writeReadyCallback(GPollableOutputStream*, void*);

// Having a list of active handles means that we do not have to worry about WebCore
// reference counting in GLib callbacks. Once the handle is off the active handles list
// we just ignore it in the callback. We avoid a lot of extra checks and tricky
// situations this way.
static HashMap<void*, SocketStreamHandle*> gActiveHandles;
COMPILE_ASSERT(HashTraits<SocketStreamHandle*>::emptyValueIsZero, emptyMapValue_is_0);

static SocketStreamHandle* getHandleFromId(void* id)
{
    return gActiveHandles.get(id);
}

static void deactivateHandle(SocketStreamHandle* handle)
{
    gActiveHandles.remove(handle->id());
}

static void* activateHandle(SocketStreamHandle* handle)
{
    // The first id cannot be 0, because it conflicts with the HashMap emptyValue.
    static gint currentHandleId = 1;
    void* id = GINT_TO_POINTER(currentHandleId++);
    gActiveHandles.set(id, handle);
    return id;
}

SocketStreamHandle::SocketStreamHandle(const KURL& url, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(url, client)
    , m_readBuffer(0)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);
    unsigned int port = url.hasPort() ? url.port() : (url.protocolIs("wss") ? 443 : 80);

    m_id = activateHandle(this);
    GRefPtr<GSocketClient> socketClient = adoptGRef(g_socket_client_new());
    if (url.protocolIs("wss"))
        g_socket_client_set_tls(socketClient.get(), TRUE);
    g_socket_client_connect_to_host_async(socketClient.get(), url.host().utf8().data(), port, 0,
        reinterpret_cast<GAsyncReadyCallback>(connectedCallback), m_id);
}

SocketStreamHandle::SocketStreamHandle(GSocketConnection* socketConnection, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(KURL(), client)
    , m_readBuffer(0)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);
    m_id = activateHandle(this);
    connected(socketConnection, 0);
}

SocketStreamHandle::~SocketStreamHandle()
{
    LOG(Network, "SocketStreamHandle %p delete", this);
    // If for some reason we were destroyed without closing, ensure that we are deactivated.
    deactivateHandle(this);
    setClient(0);
}

void SocketStreamHandle::connected(GSocketConnection* socketConnection, GError* error)
{
    if (error) {
        m_client->didFailSocketStream(this, SocketStreamError(error->code, error->message));
        return;
    }

    m_socketConnection = socketConnection;
    m_outputStream = G_POLLABLE_OUTPUT_STREAM(g_io_stream_get_output_stream(G_IO_STREAM(m_socketConnection.get())));
    m_inputStream = g_io_stream_get_input_stream(G_IO_STREAM(m_socketConnection.get()));

    m_readBuffer = new char[READ_BUFFER_SIZE];
    g_input_stream_read_async(m_inputStream.get(), m_readBuffer, READ_BUFFER_SIZE, G_PRIORITY_DEFAULT, 0,
        reinterpret_cast<GAsyncReadyCallback>(readReadyCallback), m_id);

    m_state = Open;
    m_client->didOpenSocketStream(this);
}

void SocketStreamHandle::readBytes(signed long bytesRead, GError* error)
{
    if (error) {
        m_client->didFailSocketStream(this, SocketStreamError(error->code, error->message));
        return;
    }

    if (!bytesRead) {
        close();
        return;
    }

    // The client can close the handle, potentially removing the last reference.
    RefPtr<SocketStreamHandle> protect(this); 
    m_client->didReceiveSocketStreamData(this, m_readBuffer, bytesRead);
    if (m_inputStream) // The client may have closed the connection.
        g_input_stream_read_async(m_inputStream.get(), m_readBuffer, READ_BUFFER_SIZE, G_PRIORITY_DEFAULT, 0,
            reinterpret_cast<GAsyncReadyCallback>(readReadyCallback), m_id);
}

void SocketStreamHandle::writeReady()
{
    // We no longer have buffered data, so stop waiting for the socket to be writable.
    if (!bufferedAmount()) {
        stopWaitingForSocketWritability();
        return;
    }

    sendPendingData();
}

int SocketStreamHandle::platformSend(const char* data, int length)
{
    LOG(Network, "SocketStreamHandle %p platformSend", this);
    if (!m_outputStream || !data)
        return 0;

    GOwnPtr<GError> error;
    gssize written = g_pollable_output_stream_write_nonblocking(m_outputStream.get(), data, length, 0, &error.outPtr());
    if (error) {
        if (g_error_matches(error.get(), G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
            beginWaitingForSocketWritability();
        else
            m_client->didFailSocketStream(this, SocketStreamError(error->code, error->message));
        return 0;
    }

    // If we did not send all the bytes we were given, we know that
    // SocketStreamHandleBase will need to send more in the future.
    if (written < length)
        beginWaitingForSocketWritability();

    return written;
}

void SocketStreamHandle::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);
    // We remove this handle from the active handles list first, to disable all callbacks.
    deactivateHandle(this);
    stopWaitingForSocketWritability();

    if (m_socketConnection) {
        GOwnPtr<GError> error;
        g_io_stream_close(G_IO_STREAM(m_socketConnection.get()), 0, &error.outPtr());
        if (error)
            m_client->didFailSocketStream(this, SocketStreamError(error->code, error->message));
        m_socketConnection = 0;
    }

    m_outputStream = 0;
    m_inputStream = 0;
    delete m_readBuffer;
    m_readBuffer = 0;

    m_client->didCloseSocketStream(this);
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

void SocketStreamHandle::beginWaitingForSocketWritability()
{
    if (m_writeReadySource) // Already waiting.
        return;

    m_writeReadySource = adoptGRef(g_pollable_output_stream_create_source(m_outputStream.get(), 0));
    g_source_set_callback(m_writeReadySource.get(), reinterpret_cast<GSourceFunc>(writeReadyCallback), m_id, 0);
    g_source_attach(m_writeReadySource.get(), 0);
}

void SocketStreamHandle::stopWaitingForSocketWritability()
{
    if (!m_writeReadySource) // Not waiting.
        return;

    g_source_remove(g_source_get_id(m_writeReadySource.get()));
    m_writeReadySource = 0;
}

static void connectedCallback(GSocketClient* client, GAsyncResult* result, void* id)
{
    // Always finish the connection, even if this SocketStreamHandle was deactivated earlier.
    GOwnPtr<GError> error;
    GSocketConnection* socketConnection = g_socket_client_connect_to_host_finish(client, result, &error.outPtr());

    // The SocketStreamHandle has been deactivated, so just close the connection, ignoring errors.
    SocketStreamHandle* handle = getHandleFromId(id);
    if (!handle) {
        if (socketConnection)
            g_io_stream_close(G_IO_STREAM(socketConnection), 0, 0);
        return;
    }

    handle->connected(socketConnection, error.get());
}

static void readReadyCallback(GInputStream* stream, GAsyncResult* result, void* id)
{
    // Always finish the read, even if this SocketStreamHandle was deactivated earlier.
    GOwnPtr<GError> error;
    gssize bytesRead = g_input_stream_read_finish(stream, result, &error.outPtr());

    SocketStreamHandle* handle = getHandleFromId(id);
    if (!handle)
        return;

    handle->readBytes(bytesRead, error.get());
}

static gboolean writeReadyCallback(GPollableOutputStream*, void* id)
{
    SocketStreamHandle* handle = getHandleFromId(id);
    if (!handle)
        return FALSE;

    handle->writeReady();
    return TRUE;
}

} // namespace WebCore
