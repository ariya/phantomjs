/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitSoupRequestInputStream.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Threading.h>
#include <wtf/gobject/GRefPtr.h>

struct AsyncReadData {
    AsyncReadData(GSimpleAsyncResult* result, void* buffer, gsize count, GCancellable* cancellable)
        : result(result)
        , buffer(buffer)
        , count(count)
        , cancellable(cancellable)
    {
    }

    GRefPtr<GSimpleAsyncResult> result;
    void* buffer;
    size_t count;
    GRefPtr<GCancellable> cancellable;
};

struct _WebKitSoupRequestInputStreamPrivate {
    uint64_t contentLength;
    uint64_t bytesReceived;
    uint64_t bytesRead;

    Mutex readLock;
    OwnPtr<AsyncReadData> pendingAsyncRead;
};

G_DEFINE_TYPE(WebKitSoupRequestInputStream, webkit_soup_request_input_stream, G_TYPE_MEMORY_INPUT_STREAM)

static void webkitSoupRequestInputStreamReadAsyncResultComplete(WebKitSoupRequestInputStream* stream, GSimpleAsyncResult* result, void* buffer, gsize count, GCancellable* cancellable)
{
    GError* error = 0;
    gssize bytesRead = G_INPUT_STREAM_GET_CLASS(stream)->read_fn(G_INPUT_STREAM(stream), buffer, count, cancellable, &error);
    if (!error) {
        g_simple_async_result_set_op_res_gssize(result, bytesRead);
        stream->priv->bytesRead += bytesRead;
    } else
        g_simple_async_result_take_error(result, error);
    g_simple_async_result_complete_in_idle(result);
}

static void webkitSoupRequestInputStreamPendingReadAsyncComplete(WebKitSoupRequestInputStream* stream)
{
    if (!stream->priv->pendingAsyncRead)
        return;

    AsyncReadData* data = stream->priv->pendingAsyncRead.get();
    webkitSoupRequestInputStreamReadAsyncResultComplete(stream, data->result.get(), data->buffer, data->count, data->cancellable.get());
    stream->priv->pendingAsyncRead.clear();
}

static bool webkitSoupRequestInputStreamHasDataToRead(WebKitSoupRequestInputStream* stream)
{
    return stream->priv->bytesRead < stream->priv->bytesReceived;
}

static bool webkitSoupRequestInputStreamIsWaitingForData(WebKitSoupRequestInputStream* stream)
{
    return !stream->priv->contentLength || stream->priv->bytesReceived < stream->priv->contentLength;
}

static void webkitSoupRequestInputStreamReadAsync(GInputStream* inputStream, void* buffer, gsize count, int /*priority*/, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer userData)
{
    WebKitSoupRequestInputStream* stream = WEBKIT_SOUP_REQUEST_INPUT_STREAM(inputStream);
    GRefPtr<GSimpleAsyncResult> result = adoptGRef(g_simple_async_result_new(G_OBJECT(stream), callback, userData, reinterpret_cast<void*>(webkitSoupRequestInputStreamReadAsync)));

    MutexLocker locker(stream->priv->readLock);

    if (!webkitSoupRequestInputStreamHasDataToRead(stream) && !webkitSoupRequestInputStreamIsWaitingForData(stream)) {
        g_simple_async_result_set_op_res_gssize(result.get(), 0);
        g_simple_async_result_complete_in_idle(result.get());
        return;
    }

    if (webkitSoupRequestInputStreamHasDataToRead(stream)) {
        webkitSoupRequestInputStreamReadAsyncResultComplete(stream, result.get(), buffer, count, cancellable);
        return;
    }

    stream->priv->pendingAsyncRead = adoptPtr(new AsyncReadData(result.get(), buffer, count, cancellable));
}

static gssize webkitSoupRequestInputStreamReadFinish(GInputStream*, GAsyncResult* result, GError**)
{
    GSimpleAsyncResult* simpleResult = G_SIMPLE_ASYNC_RESULT(result);
    g_warn_if_fail(g_simple_async_result_get_source_tag(simpleResult) == webkitSoupRequestInputStreamReadAsync);

    return g_simple_async_result_get_op_res_gssize(simpleResult);
}

static void webkitSoupRequestInputStreamFinalize(GObject* object)
{
    WEBKIT_SOUP_REQUEST_INPUT_STREAM(object)->priv->~WebKitSoupRequestInputStreamPrivate();
    G_OBJECT_CLASS(webkit_soup_request_input_stream_parent_class)->finalize(object);
}

static void webkit_soup_request_input_stream_init(WebKitSoupRequestInputStream* stream)
{
    WebKitSoupRequestInputStreamPrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(stream, WEBKIT_TYPE_SOUP_REQUEST_INPUT_STREAM, WebKitSoupRequestInputStreamPrivate);
    stream->priv = priv;
    new (priv) WebKitSoupRequestInputStreamPrivate();
}

static void webkit_soup_request_input_stream_class_init(WebKitSoupRequestInputStreamClass* requestStreamClass)
{
    GObjectClass* gObjectClass = G_OBJECT_CLASS(requestStreamClass);
    gObjectClass->finalize = webkitSoupRequestInputStreamFinalize;

    GInputStreamClass* inputStreamClass = G_INPUT_STREAM_CLASS(requestStreamClass);
    inputStreamClass->read_async = webkitSoupRequestInputStreamReadAsync;
    inputStreamClass->read_finish = webkitSoupRequestInputStreamReadFinish;

    g_type_class_add_private(requestStreamClass, sizeof(WebKitSoupRequestInputStreamPrivate));
}

GInputStream* webkitSoupRequestInputStreamNew(uint64_t contentLength)
{
    WebKitSoupRequestInputStream* stream = WEBKIT_SOUP_REQUEST_INPUT_STREAM(g_object_new(WEBKIT_TYPE_SOUP_REQUEST_INPUT_STREAM, NULL));
    stream->priv->contentLength = contentLength;
    return G_INPUT_STREAM(stream);
}

void webkitSoupRequestInputStreamAddData(WebKitSoupRequestInputStream* stream, const void* data, size_t dataLength)
{
    if (webkitSoupRequestInputStreamFinished(stream))
        return;

    MutexLocker locker(stream->priv->readLock);

    if (dataLength) {
        // Truncate the dataLength to the contentLength if it's known.
        if (stream->priv->contentLength && stream->priv->bytesReceived + dataLength > stream->priv->contentLength)
            dataLength = stream->priv->contentLength - stream->priv->bytesReceived;
        stream->priv->bytesReceived += dataLength;
        g_memory_input_stream_add_data(G_MEMORY_INPUT_STREAM(stream), g_memdup(data, dataLength), dataLength, g_free);
    } else {
        // We have received all the data, set contentLength to bytesReceived to indicate we have finished.
        stream->priv->contentLength = stream->priv->bytesReceived;
        // If there's a pending read to complete, read_fn will return 0 because we haven't added more data to the
        // memory input stream. And if there isn't a pending read, the next call to read_async will return 0 too, because
        // webkitSoupRequestInputStreamFinished() is now TRUE.
    }

    webkitSoupRequestInputStreamPendingReadAsyncComplete(stream);
}

bool webkitSoupRequestInputStreamFinished(WebKitSoupRequestInputStream* stream)
{
    return !webkitSoupRequestInputStreamIsWaitingForData(stream);
}
