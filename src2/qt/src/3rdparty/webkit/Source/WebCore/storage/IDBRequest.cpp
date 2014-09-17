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

#include "config.h"
#include "IDBRequest.h"

#if ENABLE(INDEXED_DATABASE)

#include "Document.h"
#include "EventException.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventQueue.h"
#include "IDBCursorWithValue.h"
#include "IDBDatabase.h"
#include "IDBEventDispatcher.h"
#include "IDBPendingTransactionMonitor.h"
#include "IDBTransaction.h"

namespace WebCore {

PassRefPtr<IDBRequest> IDBRequest::create(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, IDBTransaction* transaction)
{
    return adoptRef(new IDBRequest(context, source, transaction));
}

IDBRequest::IDBRequest(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, IDBTransaction* transaction)
    : ActiveDOMObject(context, this)
    , m_errorCode(0)
    , m_source(source)
    , m_transaction(transaction)
    , m_readyState(LOADING)
    , m_finished(false)
    , m_cursorType(IDBCursorBackendInterface::InvalidCursorType)
{
    if (m_transaction) {
        m_transaction->registerRequest(this);
        IDBPendingTransactionMonitor::removePendingTransaction(m_transaction->backend());
    }
}

IDBRequest::~IDBRequest()
{
    ASSERT(m_readyState == DONE || m_readyState == EarlyDeath || !scriptExecutionContext());
    if (m_transaction)
        m_transaction->unregisterRequest(this);
}

PassRefPtr<IDBAny> IDBRequest::result(ExceptionCode& ec) const
{
    if (m_readyState != DONE) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return 0;
    }
    return m_result;
}

unsigned short IDBRequest::errorCode(ExceptionCode& ec) const
{
    if (m_readyState != DONE) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return 0;
    }
    return m_errorCode;
}

String IDBRequest::webkitErrorMessage(ExceptionCode& ec) const
{
    if (m_readyState != DONE) {
        ec = IDBDatabaseException::NOT_ALLOWED_ERR;
        return String();
    }
    return m_errorMessage;
}

PassRefPtr<IDBAny> IDBRequest::source() const
{
    return m_source;
}

PassRefPtr<IDBTransaction> IDBRequest::transaction() const
{
    return m_transaction;
}

unsigned short IDBRequest::readyState() const
{
    ASSERT(m_readyState == LOADING || m_readyState == DONE);
    return m_readyState;
}

void IDBRequest::markEarlyDeath()
{
    ASSERT(m_readyState == LOADING);
    m_readyState = EarlyDeath;
}

bool IDBRequest::resetReadyState(IDBTransaction* transaction)
{
    ASSERT(!m_finished);
    ASSERT(scriptExecutionContext());
    ASSERT(transaction == m_transaction);
    if (m_readyState != DONE)
        return false;

    m_readyState = LOADING;
    m_result.clear();
    m_errorCode = 0;
    m_errorMessage = String();

    IDBPendingTransactionMonitor::removePendingTransaction(m_transaction->backend());

    return true;
}

IDBAny* IDBRequest::source()
{
    return m_source.get();
}

void IDBRequest::abort()
{
    if (m_readyState != LOADING) {
        ASSERT(m_readyState == DONE);
        return;
    }
    // FIXME: Remove isDocument check when
    // https://bugs.webkit.org/show_bug.cgi?id=57789 is resolved.
    if (!scriptExecutionContext() || !scriptExecutionContext()->isDocument())
        return;

    EventQueue* eventQueue = static_cast<Document*>(scriptExecutionContext())->eventQueue();
    for (size_t i = 0; i < m_enqueuedEvents.size(); ++i) {
        bool removed = eventQueue->cancelEvent(m_enqueuedEvents[i].get());
        ASSERT_UNUSED(removed, removed);
    }
    m_enqueuedEvents.clear();

    m_errorCode = 0;
    m_errorMessage = String();
    m_result.clear();
    onError(IDBDatabaseError::create(IDBDatabaseException::ABORT_ERR, "The transaction was aborted, so the request cannot be fulfilled."));
}

void IDBRequest::setCursorType(IDBCursorBackendInterface::CursorType cursorType)
{
    ASSERT(m_cursorType == IDBCursorBackendInterface::InvalidCursorType);
    m_cursorType = cursorType;
}

void IDBRequest::onError(PassRefPtr<IDBDatabaseError> error)
{
    ASSERT(!m_errorCode && m_errorMessage.isNull() && !m_result);
    m_errorCode = error->code();
    m_errorMessage = error->message();
    enqueueEvent(Event::create(eventNames().errorEvent, true, true));
}

static PassRefPtr<Event> createSuccessEvent()
{
    return Event::create(eventNames().successEvent, false, false);
}

void IDBRequest::onSuccess(PassRefPtr<IDBCursorBackendInterface> backend)
{
    ASSERT(!m_errorCode && m_errorMessage.isNull() && !m_result);
    ASSERT(m_cursorType != IDBCursorBackendInterface::InvalidCursorType);
    if (m_cursorType == IDBCursorBackendInterface::IndexKeyCursor)
        m_result = IDBAny::create(IDBCursor::create(backend, this, m_source.get(), m_transaction.get()));
    else
        m_result = IDBAny::create(IDBCursorWithValue::create(backend, this, m_source.get(), m_transaction.get()));
    enqueueEvent(createSuccessEvent());
}

void IDBRequest::onSuccess(PassRefPtr<IDBDatabaseBackendInterface> backend)
{
    ASSERT(!m_errorCode && m_errorMessage.isNull() && !m_result);
    RefPtr<IDBDatabase> idbDatabase = IDBDatabase::create(scriptExecutionContext(), backend);
    idbDatabase->open();

    m_result = IDBAny::create(idbDatabase.release());
    enqueueEvent(createSuccessEvent());
}

void IDBRequest::onSuccess(PassRefPtr<IDBKey> idbKey)
{
    ASSERT(!m_errorCode && m_errorMessage.isNull() && !m_result);
    m_result = IDBAny::create(idbKey);
    enqueueEvent(createSuccessEvent());
}

void IDBRequest::onSuccess(PassRefPtr<IDBTransactionBackendInterface> prpBackend)
{
    ASSERT(!m_errorCode && m_errorMessage.isNull() && !m_result);
    if (!scriptExecutionContext())
        return;

    RefPtr<IDBTransactionBackendInterface> backend = prpBackend;
    RefPtr<IDBTransaction> frontend = IDBTransaction::create(scriptExecutionContext(), backend, m_source->idbDatabase().get());
    backend->setCallbacks(frontend.get());
    m_transaction = frontend;

    ASSERT(m_source->type() == IDBAny::IDBDatabaseType);
    m_source->idbDatabase()->setSetVersionTransaction(frontend.get());

    IDBPendingTransactionMonitor::removePendingTransaction(m_transaction->backend());

    m_result = IDBAny::create(frontend.release());
    enqueueEvent(createSuccessEvent());
}

void IDBRequest::onSuccess(PassRefPtr<SerializedScriptValue> serializedScriptValue)
{
    ASSERT(!m_errorCode && m_errorMessage.isNull() && !m_result);
    m_result = IDBAny::create(serializedScriptValue);
    enqueueEvent(createSuccessEvent());
}

bool IDBRequest::hasPendingActivity() const
{
    // FIXME: In an ideal world, we should return true as long as anyone has a or can
    //        get a handle to us and we have event listeners. This is order to handle
    //        user generated events properly.
    return !m_finished || ActiveDOMObject::hasPendingActivity();
}

void IDBRequest::onBlocked()
{
    ASSERT_NOT_REACHED();
}

ScriptExecutionContext* IDBRequest::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

bool IDBRequest::dispatchEvent(PassRefPtr<Event> event)
{
    ASSERT(!m_finished);
    ASSERT(m_enqueuedEvents.size());
    ASSERT(scriptExecutionContext());
    ASSERT(event->target() == this);
    ASSERT(m_readyState < DONE);
    if (event->type() != eventNames().blockedEvent)
        m_readyState = DONE;

    for (size_t i = 0; i < m_enqueuedEvents.size(); ++i) {
        if (m_enqueuedEvents[i].get() == event.get())
            m_enqueuedEvents.remove(i);
    }

    Vector<RefPtr<EventTarget> > targets;
    targets.append(this);
    if (m_transaction) {
        targets.append(m_transaction);
        // If there ever are events that are associated with a database but
        // that do not have a transaction, then this will not work and we need
        // this object to actually hold a reference to the database (to ensure
        // it stays alive).
        targets.append(m_transaction->db());
    }

    // FIXME: When we allow custom event dispatching, this will probably need to change.
    ASSERT(event->type() == eventNames().successEvent || event->type() == eventNames().errorEvent || event->type() == eventNames().blockedEvent);
    bool dontPreventDefault = IDBEventDispatcher::dispatch(event.get(), targets);

    // If the result was of type IDBCursor, then we'll fire again.
    if (m_result && m_result->type() != IDBAny::IDBCursorType && m_result->type() != IDBAny::IDBCursorWithValueType)
        m_finished = true;

    if (m_transaction) {
        // If an error event and the default wasn't prevented...
        if (dontPreventDefault && event->type() == eventNames().errorEvent)
            m_transaction->backend()->abort();
        m_transaction->backend()->didCompleteTaskEvents();
    }
    return dontPreventDefault;
}

void IDBRequest::uncaughtExceptionInEventHandler()
{
    if (m_transaction)
        m_transaction->backend()->abort();
}

void IDBRequest::enqueueEvent(PassRefPtr<Event> event)
{
    ASSERT(!m_finished);
    ASSERT(m_readyState < DONE);
    if (!scriptExecutionContext())
        return;

    ASSERT(scriptExecutionContext()->isDocument());
    EventQueue* eventQueue = static_cast<Document*>(scriptExecutionContext())->eventQueue();
    event->setTarget(this);
    eventQueue->enqueueEvent(event.get());
    m_enqueuedEvents.append(event);
}

EventTargetData* IDBRequest::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* IDBRequest::ensureEventTargetData()
{
    return &m_eventTargetData;
}

} // namespace WebCore

#endif
