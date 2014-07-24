/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IDBRequest_h
#define IDBRequest_h

#if ENABLE(INDEXED_DATABASE)

#include "ActiveDOMObject.h"
#include "DOMError.h"
#include "DOMRequestState.h"
#include "DOMStringList.h"
#include "Event.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "IDBAny.h"
#include "IDBCallbacks.h"
#include "IDBCursor.h"
#include "ScriptWrappable.h"

namespace WebCore {

class IDBTransaction;

typedef int ExceptionCode;

class IDBRequest : public ScriptWrappable, public IDBCallbacks, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<IDBRequest> create(ScriptExecutionContext*, PassRefPtr<IDBAny> source, IDBTransaction*);
    static PassRefPtr<IDBRequest> create(ScriptExecutionContext*, PassRefPtr<IDBAny> source, IDBDatabaseBackendInterface::TaskType, IDBTransaction*);
    virtual ~IDBRequest();

    PassRefPtr<IDBAny> result(ExceptionCode&) const;
    unsigned short errorCode(ExceptionCode&) const;
    PassRefPtr<DOMError> error(ExceptionCode&) const;
    String webkitErrorMessage(ExceptionCode&) const;
    PassRefPtr<IDBAny> source() const;
    PassRefPtr<IDBTransaction> transaction() const;
    void preventPropagation() { m_preventPropagation = true; }

    // Defined in the IDL
    enum ReadyState {
        PENDING = 1,
        DONE = 2,
        EarlyDeath = 3
    };

    const String& readyState() const;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(success);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);

    void markEarlyDeath();
    void setCursorDetails(IndexedDB::CursorType, IndexedDB::CursorDirection);
    void setPendingCursor(PassRefPtr<IDBCursor>);
    void finishCursor();
    void abort();

    // IDBCallbacks
    virtual void onError(PassRefPtr<IDBDatabaseError>);
    virtual void onSuccess(PassRefPtr<DOMStringList>);
    virtual void onSuccess(PassRefPtr<IDBCursorBackendInterface>, PassRefPtr<IDBKey>, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SharedBuffer>);
    virtual void onSuccess(PassRefPtr<IDBKey>);
    virtual void onSuccess(PassRefPtr<SharedBuffer>);
    virtual void onSuccess(PassRefPtr<SharedBuffer>, PassRefPtr<IDBKey>, const IDBKeyPath&);
    virtual void onSuccess(int64_t);
    virtual void onSuccess();
    virtual void onSuccess(PassRefPtr<IDBKey>, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SharedBuffer>);
    virtual void onSuccessWithPrefetch(const Vector<RefPtr<IDBKey> >&, const Vector<RefPtr<IDBKey> >&, const Vector<RefPtr<SharedBuffer> >&) { ASSERT_NOT_REACHED(); } // Not implemented. Callback should not reach the renderer side.

    // ActiveDOMObject
    virtual bool hasPendingActivity() const OVERRIDE;
    virtual void stop() OVERRIDE;

    // EventTarget
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;
    virtual void uncaughtExceptionInEventHandler();

    using EventTarget::dispatchEvent;
    virtual bool dispatchEvent(PassRefPtr<Event>) OVERRIDE;

    void transactionDidFinishAndDispatch();

    using RefCounted<IDBCallbacks>::ref;
    using RefCounted<IDBCallbacks>::deref;

    IDBDatabaseBackendInterface::TaskType taskType() { return m_taskType; }

    DOMRequestState* requestState() { return &m_requestState; }

protected:
    IDBRequest(ScriptExecutionContext*, PassRefPtr<IDBAny> source, IDBDatabaseBackendInterface::TaskType, IDBTransaction*);
    void enqueueEvent(PassRefPtr<Event>);
    virtual bool shouldEnqueueEvent() const;
    void onSuccessInternal(PassRefPtr<SerializedScriptValue>);
    void onSuccessInternal(const ScriptValue&);

    RefPtr<IDBAny> m_result;
    unsigned short m_errorCode;
    String m_errorMessage;
    RefPtr<DOMError> m_error;
    bool m_contextStopped;
    RefPtr<IDBTransaction> m_transaction;
    ReadyState m_readyState;
    bool m_requestAborted; // May be aborted by transaction then receive async onsuccess; ignore vs. assert.

private:
    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    PassRefPtr<IDBCursor> getResultCursor();
    void setResultCursor(PassRefPtr<IDBCursor>, PassRefPtr<IDBKey>, PassRefPtr<IDBKey> primaryKey, const ScriptValue&);

    RefPtr<IDBAny> m_source;
    const IDBDatabaseBackendInterface::TaskType m_taskType;

    bool m_hasPendingActivity;
    Vector<RefPtr<Event> > m_enqueuedEvents;

    // Only used if the result type will be a cursor.
    IndexedDB::CursorType m_cursorType;
    IndexedDB::CursorDirection m_cursorDirection;
    bool m_cursorFinished;
    RefPtr<IDBCursor> m_pendingCursor;
    RefPtr<IDBKey> m_cursorKey;
    RefPtr<IDBKey> m_cursorPrimaryKey;
    ScriptValue m_cursorValue;
    bool m_didFireUpgradeNeededEvent;
    bool m_preventPropagation;

    EventTargetData m_eventTargetData;
    DOMRequestState m_requestState;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBRequest_h
