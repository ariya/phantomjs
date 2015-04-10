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

#include "EventException.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventQueue.h"
#include "ExceptionCodePlaceholder.h"
#include "IDBBindingUtilities.h"
#include "IDBCursorBackendInterface.h"
#include "IDBCursorWithValue.h"
#include "IDBDatabase.h"
#include "IDBEventDispatcher.h"
#include "IDBTracing.h"
#include "IDBTransaction.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

PassRefPtr<IDBRequest> IDBRequest::create(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, IDBTransaction* transaction)
{
    RefPtr<IDBRequest> request(adoptRef(new IDBRequest(context, source, IDBDatabaseBackendInterface::NormalTask, transaction)));
    request->suspendIfNeeded();
    // Requests associated with IDBFactory (open/deleteDatabase/getDatabaseNames) are not associated with transactions.
    if (transaction)
        transaction->registerRequest(request.get());
    return request.release();
}

PassRefPtr<IDBRequest> IDBRequest::create(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, IDBDatabaseBackendInterface::TaskType taskType, IDBTransaction* transaction)
{
    RefPtr<IDBRequest> request(adoptRef(new IDBRequest(context, source, taskType, transaction)));
    request->suspendIfNeeded();
    // Requests associated with IDBFactory (open/deleteDatabase/getDatabaseNames) are not associated with transactions.
    if (transaction)
        transaction->registerRequest(request.get());
    return request.release();
}

IDBRequest::IDBRequest(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, IDBDatabaseBackendInterface::TaskType taskType, IDBTransaction* transaction)
    : ActiveDOMObject(context)
    , m_result(0)
    , m_errorCode(0)
    , m_contextStopped(false)
    , m_transaction(transaction)
    , m_readyState(PENDING)
    , m_requestAborted(false)
    , m_source(source)
    , m_taskType(taskType)
    , m_hasPendingActivity(true)
    , m_cursorType(IndexedDB::CursorKeyAndValue)
    , m_cursorDirection(IndexedDB::CursorNext)
    , m_cursorFinished(false)
    , m_pendingCursor(0)
    , m_didFireUpgradeNeededEvent(false)
    , m_preventPropagation(false)
    , m_requestState(context)
{
}

IDBRequest::~IDBRequest()
{
    ASSERT(m_readyState == DONE || m_readyState == EarlyDeath || !scriptExecutionContext());
}

PassRefPtr<IDBAny> IDBRequest::result(ExceptionCode& ec) const
{
    if (m_readyState != DONE) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    return m_result;
}

PassRefPtr<DOMError> IDBRequest::error(ExceptionCode& ec) const
{
    if (m_readyState != DONE) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    return m_error;
}

unsigned short IDBRequest::errorCode(ExceptionCode& ec) const
{
    if (m_readyState != DONE) {
        ec = IDBDatabaseException::InvalidStateError;
        return 0;
    }
    return m_errorCode;
}

String IDBRequest::webkitErrorMessage(ExceptionCode& ec) const
{
    if (m_readyState != DONE) {
        ec = IDBDatabaseException::InvalidStateError;
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

const String& IDBRequest::readyState() const
{
    ASSERT(m_readyState == PENDING || m_readyState == DONE);
    DEFINE_STATIC_LOCAL(AtomicString, pending, ("pending", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(AtomicString, done, ("done", AtomicString::ConstructFromLiteral));

    if (m_readyState == PENDING)
        return pending;

    return done;
}

void IDBRequest::markEarlyDeath()
{
    ASSERT(m_readyState == PENDING);
    m_readyState = EarlyDeath;
    if (m_transaction)
        m_transaction->unregisterRequest(this);
}

void IDBRequest::abort()
{
    ASSERT(!m_requestAborted);
    if (m_contextStopped || !scriptExecutionContext())
        return;
    ASSERT(m_readyState == PENDING || m_readyState == DONE);
    if (m_readyState == DONE)
        return;

    // Enqueued events may be the only reference to this object.
    RefPtr<IDBRequest> self(this);

    EventQueue* eventQueue = scriptExecutionContext()->eventQueue();
    for (size_t i = 0; i < m_enqueuedEvents.size(); ++i) {
        bool removed = eventQueue->cancelEvent(m_enqueuedEvents[i].get());
        ASSERT_UNUSED(removed, removed);
    }
    m_enqueuedEvents.clear();

    m_errorCode = 0;
    m_error.clear();
    m_errorMessage = String();
    m_result.clear();
    onError(IDBDatabaseError::create(IDBDatabaseException::AbortError));
    m_requestAborted = true;
}

void IDBRequest::setCursorDetails(IndexedDB::CursorType cursorType, IndexedDB::CursorDirection direction)
{
    ASSERT(m_readyState == PENDING);
    ASSERT(!m_pendingCursor);
    m_cursorType = cursorType;
    m_cursorDirection = direction;
}

void IDBRequest::setPendingCursor(PassRefPtr<IDBCursor> cursor)
{
    ASSERT(m_readyState == DONE);
    ASSERT(scriptExecutionContext());
    ASSERT(m_transaction);
    ASSERT(!m_pendingCursor);
    ASSERT(cursor == getResultCursor());

    m_pendingCursor = cursor;
    m_result.clear();
    m_readyState = PENDING;
    m_errorCode = 0;
    m_error.clear();
    m_errorMessage = String();
    m_transaction->registerRequest(this);
}

PassRefPtr<IDBCursor> IDBRequest::getResultCursor()
{
    if (!m_result)
        return 0;
    if (m_result->type() == IDBAny::IDBCursorType)
        return m_result->idbCursor();
    if (m_result->type() == IDBAny::IDBCursorWithValueType)
        return m_result->idbCursorWithValue();
    return 0;
}

void IDBRequest::setResultCursor(PassRefPtr<IDBCursor> cursor, PassRefPtr<IDBKey> key, PassRefPtr<IDBKey> primaryKey, const ScriptValue& value)
{
    ASSERT(m_readyState == PENDING);
    m_cursorKey = key;
    m_cursorPrimaryKey = primaryKey;
    m_cursorValue = value;

    if (m_cursorType == IndexedDB::CursorKeyOnly) {
        m_result = IDBAny::create(cursor);
        return;
    }

    m_result = IDBAny::create(IDBCursorWithValue::fromCursor(cursor));
}

void IDBRequest::finishCursor()
{
    m_cursorFinished = true;
    if (m_readyState != PENDING)
        m_hasPendingActivity = false;
}

bool IDBRequest::shouldEnqueueEvent() const
{
    if (m_contextStopped || !scriptExecutionContext())
        return false;
    ASSERT(m_readyState == PENDING || m_readyState == DONE);
    if (m_requestAborted)
        return false;
    ASSERT(m_readyState == PENDING);
    ASSERT(!m_errorCode && m_errorMessage.isNull() && !m_error && !m_result);
    return true;
}

void IDBRequest::onError(PassRefPtr<IDBDatabaseError> error)
{
    IDB_TRACE("IDBRequest::onError()");
    if (!shouldEnqueueEvent())
        return;

    m_errorCode = error->code();
    m_errorMessage = error->message();
    m_error = DOMError::create(IDBDatabaseException::getErrorName(error->idbCode()));
    m_pendingCursor.clear();
    enqueueEvent(Event::create(eventNames().errorEvent, true, true));
}

static PassRefPtr<Event> createSuccessEvent()
{
    return Event::create(eventNames().successEvent, false, false);
}

void IDBRequest::onSuccess(PassRefPtr<DOMStringList> domStringList)
{
    IDB_TRACE("IDBRequest::onSuccess(DOMStringList)");
    if (!shouldEnqueueEvent())
        return;

    m_result = IDBAny::create(domStringList);
    enqueueEvent(createSuccessEvent());
}

void IDBRequest::onSuccess(PassRefPtr<IDBCursorBackendInterface> backend, PassRefPtr<IDBKey> key, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SharedBuffer> buffer)
{
    IDB_TRACE("IDBRequest::onSuccess(IDBCursor)");
    if (!shouldEnqueueEvent())
        return;

    DOMRequestState::Scope scope(m_requestState);
    ScriptValue value = deserializeIDBValueBuffer(requestState(), buffer);
    ASSERT(!m_pendingCursor);
    RefPtr<IDBCursor> cursor;
    switch (m_cursorType) {
    case IndexedDB::CursorKeyOnly:
        cursor = IDBCursor::create(backend, m_cursorDirection, this, m_source.get(), m_transaction.get());
        break;
    case IndexedDB::CursorKeyAndValue:
        cursor = IDBCursorWithValue::create(backend, m_cursorDirection, this, m_source.get(), m_transaction.get());
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    setResultCursor(cursor, key, primaryKey, value);

    enqueueEvent(createSuccessEvent());
}

void IDBRequest::onSuccess(PassRefPtr<IDBKey> idbKey)
{
    IDB_TRACE("IDBRequest::onSuccess(IDBKey)");
    if (!shouldEnqueueEvent())
        return;

    if (idbKey && idbKey->isValid()) {
        DOMRequestState::Scope scope(m_requestState);
        m_result = IDBAny::create(idbKeyToScriptValue(requestState(), idbKey));
    } else
        m_result = IDBAny::createInvalid();
    enqueueEvent(createSuccessEvent());
}

void IDBRequest::onSuccess(PassRefPtr<SharedBuffer> valueBuffer)
{
    IDB_TRACE("IDBRequest::onSuccess(SharedBuffer)");
    if (!shouldEnqueueEvent())
        return;

    DOMRequestState::Scope scope(m_requestState);
    ScriptValue value = deserializeIDBValueBuffer(requestState(), valueBuffer);
    onSuccessInternal(value);
}

#ifndef NDEBUG
static PassRefPtr<IDBObjectStore> effectiveObjectStore(PassRefPtr<IDBAny> source)
{
    if (source->type() == IDBAny::IDBObjectStoreType)
        return source->idbObjectStore();
    if (source->type() == IDBAny::IDBIndexType)
        return source->idbIndex()->objectStore();

    ASSERT_NOT_REACHED();
    return 0;
}
#endif

void IDBRequest::onSuccess(PassRefPtr<SharedBuffer> valueBuffer, PassRefPtr<IDBKey> prpPrimaryKey, const IDBKeyPath& keyPath)
{
    IDB_TRACE("IDBRequest::onSuccess(SharedBuffer, IDBKey, IDBKeyPath)");
    if (!shouldEnqueueEvent())
        return;

#ifndef NDEBUG
    ASSERT(keyPath == effectiveObjectStore(m_source)->keyPath());
#endif
    DOMRequestState::Scope scope(m_requestState);
    ScriptValue value = deserializeIDBValueBuffer(requestState(), valueBuffer);

    RefPtr<IDBKey> primaryKey = prpPrimaryKey;
#ifndef NDEBUG
    RefPtr<IDBKey> expectedKey = createIDBKeyFromScriptValueAndKeyPath(requestState(), value, keyPath);
    ASSERT(!expectedKey || expectedKey->isEqual(primaryKey.get()));
#endif
    bool injected = injectIDBKeyIntoScriptValue(requestState(), primaryKey, value, keyPath);
    ASSERT_UNUSED(injected, injected);
    onSuccessInternal(value);
}

void IDBRequest::onSuccess(int64_t value)
{
    IDB_TRACE("IDBRequest::onSuccess(int64_t)");
    if (!shouldEnqueueEvent())
        return;
    return onSuccessInternal(SerializedScriptValue::numberValue(value));
}

void IDBRequest::onSuccess()
{
    IDB_TRACE("IDBRequest::onSuccess()");
    if (!shouldEnqueueEvent())
        return;
    return onSuccessInternal(SerializedScriptValue::undefinedValue());
}

void IDBRequest::onSuccessInternal(PassRefPtr<SerializedScriptValue> value)
{
    ASSERT(!m_contextStopped);
    DOMRequestState::Scope scope(m_requestState);
    return onSuccessInternal(deserializeIDBValue(requestState(), value));
}

void IDBRequest::onSuccessInternal(const ScriptValue& value)
{
    m_result = IDBAny::create(value);
    if (m_pendingCursor) {
        m_pendingCursor->close();
        m_pendingCursor.clear();
    }
    enqueueEvent(createSuccessEvent());
}

void IDBRequest::onSuccess(PassRefPtr<IDBKey> key, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SharedBuffer> buffer)
{
    IDB_TRACE("IDBRequest::onSuccess(key, primaryKey, value)");
    if (!shouldEnqueueEvent())
        return;

    DOMRequestState::Scope scope(m_requestState);
    ScriptValue value = deserializeIDBValueBuffer(requestState(), buffer);
    ASSERT(m_pendingCursor);
    setResultCursor(m_pendingCursor.release(), key, primaryKey, value);
    enqueueEvent(createSuccessEvent());
}

bool IDBRequest::hasPendingActivity() const
{
    // FIXME: In an ideal world, we should return true as long as anyone has a or can
    //        get a handle to us and we have event listeners. This is order to handle
    //        user generated events properly.
    return m_hasPendingActivity && !m_contextStopped;
}

void IDBRequest::stop()
{
    if (m_contextStopped)
        return;

    m_contextStopped = true;
    m_requestState.clear();
    if (m_readyState == PENDING)
        markEarlyDeath();
}

const AtomicString& IDBRequest::interfaceName() const
{
    return eventNames().interfaceForIDBRequest;
}

ScriptExecutionContext* IDBRequest::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

bool IDBRequest::dispatchEvent(PassRefPtr<Event> event)
{
    IDB_TRACE("IDBRequest::dispatchEvent");
    ASSERT(m_readyState == PENDING);
    ASSERT(!m_contextStopped);
    ASSERT(m_hasPendingActivity);
    ASSERT(m_enqueuedEvents.size());
    ASSERT(scriptExecutionContext());
    ASSERT(event->target() == this);
    ASSERT_WITH_MESSAGE(m_readyState < DONE, "When dispatching event %s, m_readyState < DONE(%d), was %d", event->type().string().utf8().data(), DONE, m_readyState);

    DOMRequestState::Scope scope(m_requestState);

    if (event->type() != eventNames().blockedEvent)
        m_readyState = DONE;

    for (size_t i = 0; i < m_enqueuedEvents.size(); ++i) {
        if (m_enqueuedEvents[i].get() == event.get())
            m_enqueuedEvents.remove(i);
    }

    Vector<RefPtr<EventTarget> > targets;
    targets.append(this);
    if (m_transaction && !m_preventPropagation) {
        targets.append(m_transaction);
        // If there ever are events that are associated with a database but
        // that do not have a transaction, then this will not work and we need
        // this object to actually hold a reference to the database (to ensure
        // it stays alive).
        targets.append(m_transaction->db());
    }

    // Cursor properties should not updated until the success event is being dispatched.
    RefPtr<IDBCursor> cursorToNotify;
    if (event->type() == eventNames().successEvent) {
        cursorToNotify = getResultCursor();
        if (cursorToNotify) {
            cursorToNotify->setValueReady(requestState(), m_cursorKey.release(), m_cursorPrimaryKey.release(), m_cursorValue);
            m_cursorValue.clear();
        }
    }

    if (event->type() == eventNames().upgradeneededEvent) {
        ASSERT(!m_didFireUpgradeNeededEvent);
        m_didFireUpgradeNeededEvent = true;
    }

    // FIXME: When we allow custom event dispatching, this will probably need to change.
    ASSERT_WITH_MESSAGE(event->type() == eventNames().successEvent || event->type() == eventNames().errorEvent || event->type() == eventNames().blockedEvent || event->type() == eventNames().upgradeneededEvent, "event type was %s", event->type().string().utf8().data());
    const bool setTransactionActive = m_transaction && (event->type() == eventNames().successEvent || event->type() == eventNames().upgradeneededEvent || (event->type() == eventNames().errorEvent && m_errorCode != IDBDatabaseException::AbortError));

    if (setTransactionActive)
        m_transaction->setActive(true);

    bool dontPreventDefault = IDBEventDispatcher::dispatch(event.get(), targets);

    if (m_transaction) {
        if (m_readyState == DONE)
            m_transaction->unregisterRequest(this);

        // Possibly abort the transaction. This must occur after unregistering (so this request
        // doesn't receive a second error) and before deactivating (which might trigger commit).
        if (event->type() == eventNames().errorEvent && dontPreventDefault && !m_requestAborted) {
            m_transaction->setError(m_error, m_errorMessage);
            m_transaction->abort(IGNORE_EXCEPTION);
        }

        // If this was the last request in the transaction's list, it may commit here.
        if (setTransactionActive)
            m_transaction->setActive(false);
    }

    if (cursorToNotify)
        cursorToNotify->postSuccessHandlerCallback();

    if (m_readyState == DONE && (!cursorToNotify || m_cursorFinished) && event->type() != eventNames().upgradeneededEvent)
        m_hasPendingActivity = false;

    return dontPreventDefault;
}

void IDBRequest::uncaughtExceptionInEventHandler()
{
    if (m_transaction && !m_requestAborted) {
        m_transaction->setError(DOMError::create(IDBDatabaseException::getErrorName(IDBDatabaseException::AbortError)), "Uncaught exception in event handler.");
        m_transaction->abort(IGNORE_EXCEPTION);
    }
}

void IDBRequest::transactionDidFinishAndDispatch()
{
    ASSERT(m_transaction);
    ASSERT(m_transaction->isVersionChange());
    ASSERT(m_readyState == DONE);
    ASSERT(scriptExecutionContext());
    m_transaction.clear();
    m_readyState = PENDING;
}

void IDBRequest::enqueueEvent(PassRefPtr<Event> event)
{
    ASSERT(m_readyState == PENDING || m_readyState == DONE);

    if (m_contextStopped || !scriptExecutionContext())
        return;

    ASSERT_WITH_MESSAGE(m_readyState == PENDING || m_didFireUpgradeNeededEvent, "When queueing event %s, m_readyState was %d", event->type().string().utf8().data(), m_readyState);

    EventQueue* eventQueue = scriptExecutionContext()->eventQueue();
    event->setTarget(this);

    if (eventQueue->enqueueEvent(event.get()))
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
