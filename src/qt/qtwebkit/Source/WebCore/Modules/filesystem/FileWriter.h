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

#ifndef FileWriter_h
#define FileWriter_h

#if ENABLE(FILE_SYSTEM)

#include "ActiveDOMObject.h"
#include "AsyncFileWriterClient.h"
#include "EventTarget.h"
#include "FileWriterBase.h"
#include "ScriptExecutionContext.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Blob;
class ScriptExecutionContext;

class FileWriter : public FileWriterBase, public ActiveDOMObject, public EventTarget, public AsyncFileWriterClient {
public:
    static PassRefPtr<FileWriter> create(ScriptExecutionContext*);

    enum ReadyState {
        INIT = 0,
        WRITING = 1,
        DONE = 2
    };

    void write(Blob*, ExceptionCode&);
    void seek(long long position, ExceptionCode&);
    void truncate(long long length, ExceptionCode&);
    void abort(ExceptionCode&);
    ReadyState readyState() const { return m_readyState; }
    FileError* error() const { return m_error.get(); }

    // AsyncFileWriterClient
    void didWrite(long long bytes, bool complete);
    void didTruncate();
    void didFail(FileError::ErrorCode);

    // ActiveDOMObject
    virtual bool canSuspend() const;
    virtual void stop();

    // EventTarget
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const { return ActiveDOMObject::scriptExecutionContext(); }

    using RefCounted<FileWriterBase>::ref;
    using RefCounted<FileWriterBase>::deref;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(writestart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(progress);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(write);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(writeend);

private:
    enum Operation {
        OperationNone,
        OperationWrite,
        OperationTruncate,
        OperationAbort
    };

    FileWriter(ScriptExecutionContext*);

    virtual ~FileWriter();

    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData() { return &m_eventTargetData; }
    virtual EventTargetData* ensureEventTargetData() { return &m_eventTargetData; }

    void completeAbort();

    void doOperation(Operation);

    void signalCompletion(FileError::ErrorCode);

    void fireEvent(const AtomicString& type);

    void setError(FileError::ErrorCode, ExceptionCode&);

    RefPtr<FileError> m_error;
    EventTargetData m_eventTargetData;
    ReadyState m_readyState;
    Operation m_operationInProgress;
    Operation m_queuedOperation;
    long long m_bytesWritten;
    long long m_bytesToWrite;
    long long m_truncateLength;
    long long m_numAborts;
    long long m_recursionDepth;
    double m_lastProgressNotificationTimeMS;
    RefPtr<Blob> m_blobBeingWritten;
};

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // FileWriter_h
