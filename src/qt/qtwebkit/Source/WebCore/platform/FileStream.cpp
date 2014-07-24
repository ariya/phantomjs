/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
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

#include "FileStream.h"

#include "FileSystem.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

FileStream::FileStream()
    : m_handle(invalidPlatformFileHandle)
    , m_bytesProcessed(0)
    , m_totalBytesToRead(0)
{
}

FileStream::~FileStream()
{
    ASSERT(!isHandleValid(m_handle));
}

// FIXME: To be removed when we switch to using BlobData.
void FileStream::start()
{
}

void FileStream::stop()
{
    close();
}

long long FileStream::getSize(const String& path, double expectedModificationTime)
{
    // Check the modification time for the possible file change.
    time_t modificationTime;
    if (!getFileModificationTime(path, modificationTime))
        return -1;
    if (isValidFileTime(expectedModificationTime)) {
        if (static_cast<time_t>(expectedModificationTime) != modificationTime)
            return -1;
    }

    // Now get the file size.
    long long length;
    if (!getFileSize(path, length))
        return -1;

    return length;
}

bool FileStream::openForRead(const String& path, long long offset, long long length)
{
    if (isHandleValid(m_handle))
        return true;

    // Open the file.
    m_handle = openFile(path, OpenForRead);
    if (!isHandleValid(m_handle))
        return false;

    // Jump to the beginning position if the file has been sliced.
    if (offset > 0) {
        if (seekFile(m_handle, offset, SeekFromBeginning) < 0)
            return false;
    }

    m_totalBytesToRead = length;
    m_bytesProcessed = 0;

    return true;
}

bool FileStream::openForWrite(const String&)
{
    // FIXME: to be implemented.
    return false;
}

void FileStream::close()
{
    if (isHandleValid(m_handle)) {
        closeFile(m_handle);
        m_handle = invalidPlatformFileHandle;
    }
}

int FileStream::read(char* buffer, int bufferSize)
{
    if (!isHandleValid(m_handle))
        return -1;

    long long remaining = m_totalBytesToRead - m_bytesProcessed;
    int bytesToRead = (remaining < bufferSize) ? static_cast<int>(remaining) : bufferSize;
    int bytesRead = 0;
    if (bytesToRead > 0)
        bytesRead = readFromFile(m_handle, buffer, bytesToRead);
    if (bytesRead < 0)
        return -1;
    if (bytesRead > 0)
        m_bytesProcessed += bytesRead;

    return bytesRead;
}

int FileStream::write(const KURL&, long long, int)
{
    // FIXME: to be implemented.
    return -1;
}

bool FileStream::truncate(long long)
{
    // FIXME: to be implemented.
    return false;
}

} // namespace WebCore

#endif // ENABLE(BLOB)
