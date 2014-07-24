/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "History.h"
#include "JSHistory.h"
#include "JSPopStateEvent.h"

using namespace JSC;

namespace WebCore {

// Save the state value to the m_state member of a JSPopStateEvent, and return it, for convenience.
static const JSValue& cacheState(ExecState* exec, JSPopStateEvent* event, const JSValue& state)
{
    event->m_state.set(exec->vm(), event, state);
    return state;
}

JSValue JSPopStateEvent::state(ExecState* exec) const
{
    JSValue cachedValue = m_state.get();
    if (!cachedValue.isEmpty())
        return cachedValue;

    PopStateEvent* event = static_cast<PopStateEvent*>(impl());

    if (!event->state().hasNoValue())
        return cacheState(exec, const_cast<JSPopStateEvent*>(this), event->state().jsValue());

    History* history = event->history();
    if (!history || !event->serializedState())
        return cacheState(exec, const_cast<JSPopStateEvent*>(this), jsNull());

    // There's no cached value from a previous invocation, nor a state value was provided by the
    // event, but there is a history object, so first we need to see if the state object has been
    // deserialized through the history object already.
    // The current history state object might've changed in the meantime, so we need to take care
    // of using the correct one, and always share the same deserialization with history.state.

    bool isSameState = history->isSameAsCurrentState(event->serializedState().get());
    JSValue result;

    if (isSameState) {
        JSHistory* jsHistory = jsCast<JSHistory*>(toJS(exec, globalObject(), history).asCell());
        result = jsHistory->state(exec);
    } else
        result = event->serializedState()->deserialize(exec, globalObject(), 0);

    return cacheState(exec, const_cast<JSPopStateEvent*>(this), result);
}

} // namespace WebCore
