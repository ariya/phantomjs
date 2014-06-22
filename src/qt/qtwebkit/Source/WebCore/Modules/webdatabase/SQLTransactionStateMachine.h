/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef SQLTransactionStateMachine_h
#define SQLTransactionStateMachine_h

#if ENABLE(SQL_DATABASE)

#include "SQLTransactionState.h"
#include <wtf/ThreadSafeRefCounted.h>

namespace WebCore {

template<typename T>
class SQLTransactionStateMachine {
public:
    virtual ~SQLTransactionStateMachine() { }

protected:
    SQLTransactionStateMachine();

    typedef SQLTransactionState (T::* StateFunction)();
    virtual StateFunction stateFunctionFor(SQLTransactionState) = 0;

    void setStateToRequestedState();
    void runStateMachine();

    SQLTransactionState m_nextState;
    SQLTransactionState m_requestedState;

#ifndef NDEBUG
    // The state audit trail (i.e. bread crumbs) keeps track of up to the last
    // s_sizeOfStateAuditTrail states that the state machine enters. The audit
    // trail is updated before entering each state. This is for debugging use
    // only.
    static const int s_sizeOfStateAuditTrail = 20;
    int m_nextStateAuditEntry;
    SQLTransactionState m_stateAuditTrail[s_sizeOfStateAuditTrail];
#endif
};

#if !LOG_DISABLED
extern const char* nameForSQLTransactionState(SQLTransactionState);
#endif

template<typename T>
SQLTransactionStateMachine<T>::SQLTransactionStateMachine()
    : m_nextState(SQLTransactionState::Idle)
    , m_requestedState(SQLTransactionState::Idle)
#ifndef NDEBUG
    , m_nextStateAuditEntry(0)
#endif
{
#ifndef NDEBUG
    for (int i = 0; i < s_sizeOfStateAuditTrail; i++)
        m_stateAuditTrail[i] = SQLTransactionState::NumberOfStates;
#endif
}

template<typename T>
void SQLTransactionStateMachine<T>::setStateToRequestedState()
{
    ASSERT(m_nextState == SQLTransactionState::Idle);
    ASSERT(m_requestedState != SQLTransactionState::Idle);
    m_nextState = m_requestedState;
    m_requestedState = SQLTransactionState::Idle;
}

template<typename T>
void SQLTransactionStateMachine<T>::runStateMachine()
{
    ASSERT(SQLTransactionState::End < SQLTransactionState::Idle);
    while (m_nextState > SQLTransactionState::Idle) {
        ASSERT(m_nextState < SQLTransactionState::NumberOfStates);
        StateFunction stateFunction = stateFunctionFor(m_nextState);
        ASSERT(stateFunction);

#ifndef NDEBUG
        m_stateAuditTrail[m_nextStateAuditEntry] = m_nextState;
        m_nextStateAuditEntry = (m_nextStateAuditEntry + 1) % s_sizeOfStateAuditTrail;
#endif
        m_nextState = (static_cast<T*>(this)->*stateFunction)();
    }
}

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // SQLTransactionStateMachine_h
