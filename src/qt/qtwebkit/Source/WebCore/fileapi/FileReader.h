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

#ifndef FileReader_h
#define FileReader_h

#if ENABLE(BLOB)

#include "ActiveDOMObject.h"
#include "EventTarget.h"
#include "FileError.h"
#include "FileReaderLoader.h"
#include "FileReaderLoaderClient.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Blob;
class ScriptExecutionContext;

typedef int ExceptionCode;

class FileReader : public RefCounted<FileReader>, public ActiveDOMObject, public EventTarget, public FileReaderLoaderClient {
public:
    static PassRefPtr<FileReader> create(ScriptExecutionContext*);

    virtual ~FileReader();

    enum ReadyState {
        EMPTY = 0,
        LOADING = 1,
        DONE = 2
    };

    void readAsArrayBuffer(Blob*, ExceptionCode&);
    void readAsBinaryString(Blob*, ExceptionCode&);
    void readAsText(Blob*, const String& encoding, ExceptionCode&);
    void readAsText(Blob*, ExceptionCode&);
    void readAsDataURL(Blob*, ExceptionCode&);
    void abort();

    void doAbort();

    ReadyState readyState() const { return m_state; }
    PassRefPtr<FileError> error() { return m_error; }
    FileReaderLoader::ReadType readType() const { return m_readType; }
    PassRefPtr<ArrayBuffer> arrayBufferResult() const;
    String stringResult();

    // ActiveDOMObject
    virtual bool canSuspend() const;
    virtual void stop();

    // EventTarget
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const { return ActiveDOMObject::scriptExecutionContext(); }

    // FileReaderLoaderClient
    virtual void didStartLoading();
    virtual void didReceiveData();
    virtual void didFinishLoading();
    virtual void didFail(int errorCode);

    using RefCounted<FileReader>::ref;
    using RefCounted<FileReader>::deref;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(loadstart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(progress);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(load);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(loadend);

private:
    FileReader(ScriptExecutionContext*);

    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData() { return &m_eventTargetData; }
    virtual EventTargetData* ensureEventTargetData() { return &m_eventTargetData; }

    void terminate();
    void readInternal(Blob*, FileReaderLoader::ReadType, ExceptionCode&);
    void fireErrorEvent(int httpStatusCode);
    void fireEvent(const AtomicString& type);

    ReadyState m_state;
    bool m_aborting;
    EventTargetData m_eventTargetData;

    RefPtr<Blob> m_blob;
    FileReaderLoader::ReadType m_readType;
    String m_encoding;

    OwnPtr<FileReaderLoader> m_loader;
    RefPtr<FileError> m_error;
    double m_lastProgressNotificationTimeMS;
};

} // namespace WebCore

#endif // ENABLE(BLOB)

#endif // FileReader_h
