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

#include "FileReader.h"

#include "CrossThreadTask.h"
#include "ExceptionCode.h"
#include "File.h"
#include "Logging.h"
#include "ProgressEvent.h"
#include "ScriptExecutionContext.h"
#include <wtf/ArrayBuffer.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>

namespace WebCore {

static const double progressNotificationIntervalMS = 50;

PassRefPtr<FileReader> FileReader::create(ScriptExecutionContext* context)
{
    RefPtr<FileReader> fileReader(adoptRef(new FileReader(context)));
    fileReader->suspendIfNeeded();
    return fileReader.release();
}

FileReader::FileReader(ScriptExecutionContext* context)
    : ActiveDOMObject(context)
    , m_state(EMPTY)
    , m_aborting(false)
    , m_readType(FileReaderLoader::ReadAsBinaryString)
    , m_lastProgressNotificationTimeMS(0)
{
}

FileReader::~FileReader()
{
    terminate();
}

const AtomicString& FileReader::interfaceName() const
{
    return eventNames().interfaceForFileReader;
}

bool FileReader::canSuspend() const
{
    // FIXME: It is not currently possible to suspend a FileReader, so pages with FileReader can not go into page cache.
    return false;
}

void FileReader::stop()
{
    terminate();
}

void FileReader::readAsArrayBuffer(Blob* blob, ExceptionCode& ec)
{
    if (!blob)
        return;

    LOG(FileAPI, "FileReader: reading as array buffer: %s %s\n", blob->url().string().utf8().data(), blob->isFile() ? toFile(blob)->path().utf8().data() : "");

    readInternal(blob, FileReaderLoader::ReadAsArrayBuffer, ec);
}

void FileReader::readAsBinaryString(Blob* blob, ExceptionCode& ec)
{
    if (!blob)
        return;

    LOG(FileAPI, "FileReader: reading as binary: %s %s\n", blob->url().string().utf8().data(), blob->isFile() ? toFile(blob)->path().utf8().data() : "");

    readInternal(blob, FileReaderLoader::ReadAsBinaryString, ec);
}

void FileReader::readAsText(Blob* blob, const String& encoding, ExceptionCode& ec)
{
    if (!blob)
        return;

    LOG(FileAPI, "FileReader: reading as text: %s %s\n", blob->url().string().utf8().data(), blob->isFile() ? toFile(blob)->path().utf8().data() : "");

    m_encoding = encoding;
    readInternal(blob, FileReaderLoader::ReadAsText, ec);
}

void FileReader::readAsText(Blob* blob, ExceptionCode& ec)
{
    readAsText(blob, String(), ec);
}

void FileReader::readAsDataURL(Blob* blob, ExceptionCode& ec)
{
    if (!blob)
        return;

    LOG(FileAPI, "FileReader: reading as data URL: %s %s\n", blob->url().string().utf8().data(), blob->isFile() ? toFile(blob)->path().utf8().data() : "");

    readInternal(blob, FileReaderLoader::ReadAsDataURL, ec);
}

void FileReader::readInternal(Blob* blob, FileReaderLoader::ReadType type, ExceptionCode& ec)
{
    // If multiple concurrent read methods are called on the same FileReader, INVALID_STATE_ERR should be thrown when the state is LOADING.
    if (m_state == LOADING) {
        ec = INVALID_STATE_ERR;
        return;
    }

    setPendingActivity(this);

    m_blob = blob;
    m_readType = type;
    m_state = LOADING;
    m_error = 0;

    m_loader = adoptPtr(new FileReaderLoader(m_readType, this));
    m_loader->setEncoding(m_encoding);
    m_loader->setDataType(m_blob->type());
    m_loader->start(scriptExecutionContext(), m_blob.get());
}

static void delayedAbort(ScriptExecutionContext*, FileReader* reader)
{
    reader->doAbort();
}

void FileReader::abort()
{
    LOG(FileAPI, "FileReader: aborting\n");

    if (m_aborting || m_state != LOADING)
        return;
    m_aborting = true;

    // Schedule to have the abort done later since abort() might be called from the event handler and we do not want the resource loading code to be in the stack.
    scriptExecutionContext()->postTask(
        createCallbackTask(&delayedAbort, AllowAccessLater(this)));
}

void FileReader::doAbort()
{
    ASSERT(m_state != DONE);

    terminate();
    m_aborting = false;

    m_error = FileError::create(FileError::ABORT_ERR);

    fireEvent(eventNames().errorEvent);
    fireEvent(eventNames().abortEvent);
    fireEvent(eventNames().loadendEvent);

    // All possible events have fired and we're done, no more pending activity.
    unsetPendingActivity(this);
}

void FileReader::terminate()
{
    if (m_loader) {
        m_loader->cancel();
        m_loader = nullptr;
    }
    m_state = DONE;
}

void FileReader::didStartLoading()
{
    fireEvent(eventNames().loadstartEvent);
}

void FileReader::didReceiveData()
{
    // Fire the progress event at least every 50ms.
    double now = currentTimeMS();
    if (!m_lastProgressNotificationTimeMS)
        m_lastProgressNotificationTimeMS = now;
    else if (now - m_lastProgressNotificationTimeMS > progressNotificationIntervalMS) {
        fireEvent(eventNames().progressEvent);
        m_lastProgressNotificationTimeMS = now;
    }
}

void FileReader::didFinishLoading()
{
    if (m_aborting)
        return;

    ASSERT(m_state != DONE);
    m_state = DONE;

    fireEvent(eventNames().progressEvent);
    fireEvent(eventNames().loadEvent);
    fireEvent(eventNames().loadendEvent);
    
    // All possible events have fired and we're done, no more pending activity.
    unsetPendingActivity(this);
}

void FileReader::didFail(int errorCode)
{
    // If we're aborting, do not proceed with normal error handling since it is covered in aborting code.
    if (m_aborting)
        return;

    ASSERT(m_state != DONE);
    m_state = DONE;

    m_error = FileError::create(static_cast<FileError::ErrorCode>(errorCode));
    fireEvent(eventNames().errorEvent);
    fireEvent(eventNames().loadendEvent);
    
    // All possible events have fired and we're done, no more pending activity.
    unsetPendingActivity(this);
}

void FileReader::fireEvent(const AtomicString& type)
{
    dispatchEvent(ProgressEvent::create(type, true, m_loader ? m_loader->bytesLoaded() : 0, m_loader ? m_loader->totalBytes() : 0));
}

PassRefPtr<ArrayBuffer> FileReader::arrayBufferResult() const
{
    if (!m_loader || m_error)
        return 0;
    return m_loader->arrayBufferResult();
}

String FileReader::stringResult()
{
    if (!m_loader || m_error)
        return String();
    return m_loader->stringResult();
}

} // namespace WebCore
 
#endif // ENABLE(BLOB)
