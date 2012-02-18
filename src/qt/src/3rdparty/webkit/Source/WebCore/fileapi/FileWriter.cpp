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

#include "FileWriter.h"

#include "AsyncFileWriter.h"
#include "Blob.h"
#include "ExceptionCode.h"
#include "FileError.h"
#include "FileException.h"
#include "ProgressEvent.h"

namespace WebCore {

FileWriter::FileWriter(ScriptExecutionContext* context)
    : ActiveDOMObject(context, this)
    , m_readyState(INIT)
    , m_startedWriting(false)
    , m_bytesWritten(0)
    , m_bytesToWrite(0)
    , m_truncateLength(-1)
{
}

FileWriter::~FileWriter()
{
    if (m_readyState == WRITING)
        stop();
}

bool FileWriter::hasPendingActivity() const
{
    return m_readyState == WRITING || ActiveDOMObject::hasPendingActivity();
}

bool FileWriter::canSuspend() const
{
    // FIXME: It is not currently possible to suspend a FileWriter, so pages with FileWriter can not go into page cache.
    return false;
}

void FileWriter::stop()
{
    if (writer() && m_readyState == WRITING)
        writer()->abort();
    m_blobBeingWritten.clear();
    m_readyState = DONE;
}

void FileWriter::write(Blob* data, ExceptionCode& ec)
{
    ASSERT(writer());
    if (m_readyState == WRITING) {
        setError(FileError::INVALID_STATE_ERR, ec);
        return;
    }
    if (!data) {
        setError(FileError::TYPE_MISMATCH_ERR, ec);
        return;
    }

    m_blobBeingWritten = data;
    m_readyState = WRITING;
    m_startedWriting = false;
    m_bytesWritten = 0;
    m_bytesToWrite = data->size();
    writer()->write(position(), data);
}

void FileWriter::seek(long long position, ExceptionCode& ec)
{
    ASSERT(writer());
    if (m_readyState == WRITING) {
        setError(FileError::INVALID_STATE_ERR, ec);
        return;
    }

    m_bytesWritten = 0;
    m_bytesToWrite = 0;
    seekInternal(position);
}

void FileWriter::truncate(long long position, ExceptionCode& ec)
{
    ASSERT(writer());
    if (m_readyState == WRITING || position < 0) {
        setError(FileError::INVALID_STATE_ERR, ec);
        return;
    }
    m_readyState = WRITING;
    m_bytesWritten = 0;
    m_bytesToWrite = 0;
    m_truncateLength = position;
    writer()->truncate(position);
}

void FileWriter::abort(ExceptionCode& ec)
{
    ASSERT(writer());
    if (m_readyState != WRITING) {
        setError(FileError::INVALID_STATE_ERR, ec);
        return;
    }

    writer()->abort();
}

void FileWriter::didWrite(long long bytes, bool complete)
{
    ASSERT(bytes + m_bytesWritten > 0);
    ASSERT(bytes + m_bytesWritten <= m_bytesToWrite);
    if (!m_startedWriting) {
        fireEvent(eventNames().writestartEvent);
        m_startedWriting = true;
    }
    m_bytesWritten += bytes;
    ASSERT((m_bytesWritten == m_bytesToWrite) || !complete);
    setPosition(position() + bytes);
    if (position() > length())
        setLength(position());
    fireEvent(eventNames().progressEvent);
    if (complete) {
        m_blobBeingWritten.clear();
        scriptExecutionContext()->postTask(FileWriterCompletionEventTask::create(this, FileError::OK));
    }
}

void FileWriter::didTruncate()
{
    ASSERT(m_truncateLength >= 0);
    fireEvent(eventNames().writestartEvent);
    setLength(m_truncateLength);
    if (position() > length())
        setPosition(length());
    m_truncateLength = -1;
    scriptExecutionContext()->postTask(FileWriterCompletionEventTask::create(this, FileError::OK));
}

void FileWriter::didFail(FileError::ErrorCode code)
{
    ASSERT(code != FileError::OK);
    m_blobBeingWritten.clear();
    scriptExecutionContext()->postTask(FileWriterCompletionEventTask::create(this, code));
}

void FileWriter::signalCompletion(FileError::ErrorCode code)
{
    m_readyState = DONE;
    if (FileError::OK != code) {
        m_error = FileError::create(code);
        fireEvent(eventNames().errorEvent);
        if (FileError::ABORT_ERR == code)
            fireEvent(eventNames().abortEvent);
    } else
        fireEvent(eventNames().writeEvent);
    fireEvent(eventNames().writeendEvent);
}

void FileWriter::fireEvent(const AtomicString& type)
{
    dispatchEvent(ProgressEvent::create(type, true, m_bytesWritten, m_bytesToWrite));
}

void FileWriter::setError(FileError::ErrorCode errorCode, ExceptionCode& ec)
{
    ec = FileException::ErrorCodeToExceptionCode(errorCode);
    m_error = FileError::create(errorCode);
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
