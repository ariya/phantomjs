/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#if ENABLE(INSPECTOR)

#include "InspectorHistory.h"

#include "ExceptionCodePlaceholder.h"
#include "Node.h"

namespace WebCore {

namespace {

class UndoableStateMark : public InspectorHistory::Action {
public:
    UndoableStateMark() : InspectorHistory::Action("[UndoableState]") { }

    virtual bool perform(ExceptionCode&) { return true; }

    virtual bool undo(ExceptionCode&) { return true; }

    virtual bool redo(ExceptionCode&) { return true; }

    virtual bool isUndoableStateMark() { return true; }
};

}

InspectorHistory::Action::Action(const String& name) : m_name(name)
{
}

InspectorHistory::Action::~Action()
{
}

String InspectorHistory::Action::toString()
{
    return m_name;
}

bool InspectorHistory::Action::isUndoableStateMark()
{
    return false;
}

String InspectorHistory::Action::mergeId()
{
    return "";
}

void InspectorHistory::Action::merge(PassOwnPtr<Action>)
{
}

InspectorHistory::InspectorHistory() : m_afterLastActionIndex(0) { }

InspectorHistory::~InspectorHistory() { }

bool InspectorHistory::perform(PassOwnPtr<Action> action, ExceptionCode& ec)
{
    if (!action->perform(ec))
        return false;

    if (!action->mergeId().isEmpty() && m_afterLastActionIndex > 0 && action->mergeId() == m_history[m_afterLastActionIndex - 1]->mergeId())
        m_history[m_afterLastActionIndex - 1]->merge(action);
    else {
        m_history.resize(m_afterLastActionIndex);
        m_history.append(action);
        ++m_afterLastActionIndex;
    }
    return true;
}

void InspectorHistory::markUndoableState()
{
    perform(adoptPtr(new UndoableStateMark()), IGNORE_EXCEPTION);
}

bool InspectorHistory::undo(ExceptionCode& ec)
{
    while (m_afterLastActionIndex > 0 && m_history[m_afterLastActionIndex - 1]->isUndoableStateMark())
        --m_afterLastActionIndex;

    while (m_afterLastActionIndex > 0) {
        Action* action = m_history[m_afterLastActionIndex - 1].get();
        if (!action->undo(ec)) {
            reset();
            return false;
        }
        --m_afterLastActionIndex;
        if (action->isUndoableStateMark())
            break;
    }

    return true;
}

bool InspectorHistory::redo(ExceptionCode& ec)
{
    while (m_afterLastActionIndex < m_history.size() && m_history[m_afterLastActionIndex]->isUndoableStateMark())
        ++m_afterLastActionIndex;

    while (m_afterLastActionIndex < m_history.size()) {
        Action* action = m_history[m_afterLastActionIndex].get();
        if (!action->redo(ec)) {
            reset();
            return false;
        }
        ++m_afterLastActionIndex;
        if (action->isUndoableStateMark())
            break;
    }
    return true;
}

void InspectorHistory::reset()
{
    m_afterLastActionIndex = 0;
    m_history.clear();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
