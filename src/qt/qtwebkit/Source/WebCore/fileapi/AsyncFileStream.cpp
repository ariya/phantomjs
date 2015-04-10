/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#if ENABLE(BLOB)

#include "AsyncFileStream.h"

#include "Blob.h"
#include "FileStream.h"
#include "FileStreamClient.h"
#include "FileThread.h"
#include "FileThreadTask.h"
#include "MainThreadTask.h"
#include <wtf/MainThread.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static PassRefPtr<FileThread> createFileThread()
{
    RefPtr<FileThread> thread = FileThread::create();
    if (!thread->start())
        return 0;
    return thread.release();
}

static FileThread* fileThread()
{
    ASSERT(isMainThread());
    static FileThread* thread = createFileThread().leakRef();
    return thread;
}

inline AsyncFileStream::AsyncFileStream(FileStreamClient* client)
    : m_stream(FileStream::create())
    , m_client(client)
{
    ASSERT(isMainThread());
}

PassRefPtr<AsyncFileStream> AsyncFileStream::create(FileStreamClient* client)
{
    RefPtr<AsyncFileStream> proxy = adoptRef(new AsyncFileStream(client));

    // Hold a reference so that the instance will not get deleted while there are tasks on the file thread.
    // This is balanced by the deref in derefProxyOnContext below.
    proxy->ref();

    fileThread()->postTask(createFileThreadTask(proxy.get(), &AsyncFileStream::startOnFileThread));

    return proxy.release();
}

AsyncFileStream::~AsyncFileStream()
{
}

static void didStart(AsyncFileStream* proxy)
{
    if (proxy->client())
        proxy->client()->didStart();
}

void AsyncFileStream::startOnFileThread()
{
    // FIXME: It is not correct to check m_client from a secondary thread - stop() could be racing with this check.
    if (!m_client)
        return;
    m_stream->start();
    callOnMainThread(didStart, AllowCrossThreadAccess(this));
}

void AsyncFileStream::stop()
{
    // Clear the client so that we won't be invoking callbacks on the client.
    setClient(0);

    fileThread()->unscheduleTasks(m_stream.get());
    fileThread()->postTask(createFileThreadTask(this, &AsyncFileStream::stopOnFileThread));
}

static void derefProxyOnMainThread(AsyncFileStream* proxy)
{
    ASSERT(proxy->hasOneRef());
    proxy->deref();
}

void AsyncFileStream::stopOnFileThread()
{
    m_stream->stop();
    callOnMainThread(derefProxyOnMainThread, AllowCrossThreadAccess(this));
}

static void didGetSize(AsyncFileStream* proxy, long long size)
{
    if (proxy->client())
        proxy->client()->didGetSize(size);
}

void AsyncFileStream::getSize(const String& path, double expectedModificationTime)
{
    fileThread()->postTask(createFileThreadTask(this, &AsyncFileStream::getSizeOnFileThread, path, expectedModificationTime));
}

void AsyncFileStream::getSizeOnFileThread(const String& path, double expectedModificationTime)
{
    long long size = m_stream->getSize(path, expectedModificationTime);
    callOnMainThread(didGetSize, AllowCrossThreadAccess(this), size);
}

static void didOpen(AsyncFileStream* proxy, bool success)
{
    if (proxy->client())
        proxy->client()->didOpen(success);
}

void AsyncFileStream::openForRead(const String& path, long long offset, long long length)
{
    fileThread()->postTask(createFileThreadTask(this, &AsyncFileStream::openForReadOnFileThread, path, offset, length));
}

void AsyncFileStream::openForReadOnFileThread(const String& path, long long offset, long long length)
{
    bool success = m_stream->openForRead(path, offset, length);
    callOnMainThread(didOpen, AllowCrossThreadAccess(this), success);
}

void AsyncFileStream::openForWrite(const String& path)
{
    fileThread()->postTask(
        createFileThreadTask(this,
                             &AsyncFileStream::openForWriteOnFileThread, path));
}

void AsyncFileStream::openForWriteOnFileThread(const String& path)
{
    bool success = m_stream->openForWrite(path);
    callOnMainThread(didOpen, AllowCrossThreadAccess(this), success);
}

void AsyncFileStream::close()
{
    fileThread()->postTask(createFileThreadTask(this, &AsyncFileStream::closeOnFileThread));
}

void AsyncFileStream::closeOnFileThread()
{
    m_stream->close();
}

static void didRead(AsyncFileStream* proxy, int bytesRead)
{
    if (proxy->client())
        proxy->client()->didRead(bytesRead);
}

void AsyncFileStream::read(char* buffer, int length)
{
    fileThread()->postTask(
        createFileThreadTask(this, &AsyncFileStream::readOnFileThread,
                             AllowCrossThreadAccess(buffer), length));
}

void AsyncFileStream::readOnFileThread(char* buffer, int length)
{
    int bytesRead = m_stream->read(buffer, length);
    callOnMainThread(didRead, AllowCrossThreadAccess(this), bytesRead);
}

static void didWrite(AsyncFileStream* proxy, int bytesWritten)
{
    if (proxy->client())
        proxy->client()->didWrite(bytesWritten);
}

void AsyncFileStream::write(const KURL& blobURL, long long position, int length)
{
    fileThread()->postTask(createFileThreadTask(this, &AsyncFileStream::writeOnFileThread, blobURL, position, length));
}

void AsyncFileStream::writeOnFileThread(const KURL& blobURL, long long position, int length)
{
    int bytesWritten = m_stream->write(blobURL, position, length);
    callOnMainThread(didWrite, AllowCrossThreadAccess(this), bytesWritten);
}

static void didTruncate(AsyncFileStream* proxy, bool success)
{
    if (proxy->client())
        proxy->client()->didTruncate(success);
}

void AsyncFileStream::truncate(long long position)
{
    fileThread()->postTask(createFileThreadTask(this, &AsyncFileStream::truncateOnFileThread, position));
}

void AsyncFileStream::truncateOnFileThread(long long position)
{
    bool success = m_stream->truncate(position);
    callOnMainThread(didTruncate, AllowCrossThreadAccess(this), success);
}

} // namespace WebCore

#endif // ENABLE(BLOB)
