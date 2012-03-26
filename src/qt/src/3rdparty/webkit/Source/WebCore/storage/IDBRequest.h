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
#include "Event.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "ExceptionCode.h"
#include "IDBAny.h"
#include "IDBCallbacks.h"

namespace WebCore {

class IDBTransaction;

class IDBRequest : public IDBCallbacks, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<IDBRequest> create(ScriptExecutionContext*, PassRefPtr<IDBAny> source, IDBTransaction*);
    virtual ~IDBRequest();

    PassRefPtr<IDBAny> result(ExceptionCode&) const;
    unsigned short errorCode(ExceptionCode&) const;
    String webkitErrorMessage(ExceptionCode&) const;
    PassRefPtr<IDBAny> source() const;
    PassRefPtr<IDBTransaction> transaction() const;

    // Defined in the IDL
    enum ReadyState {
        LOADING = 1,
        DONE = 2,
        EarlyDeath = 3
    };
    unsigned short readyState() const;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(success);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);

    void markEarlyDeath();
    bool resetReadyState(IDBTransaction*);
    void setCursorType(IDBCursorBackendInterface::CursorType);
    IDBAny* source();
    void abort();

    // IDBCallbacks
    virtual void onError(PassRefPtr<IDBDatabaseError>);
    virtual void onSuccess(PassRefPtr<IDBDatabaseBackendInterface>);
    virtual void onSuccess(PassRefPtr<IDBCursorBackendInterface>);
    virtual void onSuccess(PassRefPtr<IDBKey>);
    virtual void onSuccess(PassRefPtr<IDBTransactionBackendInterface>);
    virtual void onSuccess(PassRefPtr<SerializedScriptValue>);
    virtual void onBlocked();

    // ActiveDOMObject
    virtual bool hasPendingActivity() const;

    // EventTarget
    virtual IDBRequest* toIDBRequest() { return this; }
    virtual ScriptExecutionContext* scriptExecutionContext() const;
    virtual bool dispatchEvent(PassRefPtr<Event>);
    bool dispatchEvent(PassRefPtr<Event> event, ExceptionCode& ec) { return EventTarget::dispatchEvent(event, ec); }
    virtual void uncaughtExceptionInEventHandler();

    using ThreadSafeRefCounted<IDBCallbacks>::ref;
    using ThreadSafeRefCounted<IDBCallbacks>::deref;

protected:
    IDBRequest(ScriptExecutionContext*, PassRefPtr<IDBAny> source, IDBTransaction*);
    void enqueueEvent(PassRefPtr<Event>);
    RefPtr<IDBAny> m_result;
    unsigned short m_errorCode;
    String m_errorMessage;

private:
    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    RefPtr<IDBAny> m_source;
    RefPtr<IDBTransaction> m_transaction;

    ReadyState m_readyState;
    bool m_finished; // Is it possible that we'll fire any more events? If not, we're finished.
    Vector<RefPtr<Event> > m_enqueuedEvents;

    // Only used if the result type will be a cursor.
    IDBCursorBackendInterface::CursorType m_cursorType;

    EventTargetData m_eventTargetData;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBRequest_h
