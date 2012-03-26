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

#if ENABLE(FILE_SYSTEM)

#include "FileWriterSync.h"

#include "AsyncFileWriter.h"
#include "Blob.h"
#include "FileException.h"

namespace WebCore {

void FileWriterSync::write(Blob* data, ExceptionCode& ec)
{
    ASSERT(writer());
    ASSERT(m_complete);
    ec = 0;
    if (!data) {
        ec = FileException::TYPE_MISMATCH_ERR;
        return;
    }

    prepareForWrite();
    writer()->write(position(), data);
    writer()->waitForOperationToComplete();
    ASSERT(m_complete);
    ec = FileException::ErrorCodeToExceptionCode(m_error);
    if (ec)
        return;
    setPosition(position() + data->size());
    if (position() > length())
        setLength(position());
}

void FileWriterSync::seek(long long position, ExceptionCode& ec)
{
    ASSERT(writer());
    ASSERT(m_complete);
    ec = 0;
    seekInternal(position);
}

void FileWriterSync::truncate(long long offset, ExceptionCode& ec)
{
    ASSERT(writer());
    ASSERT(m_complete);
    ec = 0;
    if (offset < 0) {
        ec = FileException::INVALID_STATE_ERR;
        return;
    }
    prepareForWrite();
    writer()->truncate(offset);
    writer()->waitForOperationToComplete();
    ASSERT(m_complete);
    ec = FileException::ErrorCodeToExceptionCode(m_error);
    if (ec)
        return;
    if (offset < position())
        setPosition(offset);
    setLength(offset);
}

void FileWriterSync::didWrite(long long bytes, bool complete)
{
    ASSERT(m_error == FileError::OK);
    ASSERT(!m_complete);
#ifndef NDEBUG
    m_complete = complete;
#else
    ASSERT_UNUSED(complete, complete);
#endif
}

void FileWriterSync::didTruncate()
{
    ASSERT(m_error == FileError::OK);
    ASSERT(!m_complete);
#ifndef NDEBUG
    m_complete = true;
#endif
}

void FileWriterSync::didFail(FileError::ErrorCode error)
{
    ASSERT(m_error == FileError::OK);
    m_error = error;
    ASSERT(!m_complete);
#ifndef NDEBUG
    m_complete = true;
#endif
}

FileWriterSync::FileWriterSync()
    : m_error(FileError::OK)
#ifndef NDEBUG
    , m_complete(true)
#endif
{
}

void FileWriterSync::prepareForWrite()
{
    ASSERT(m_complete);
    m_error = FileError::OK;
#ifndef NDEBUG
    m_complete = false;
#endif
}

FileWriterSync::~FileWriterSync()
{
}


} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
