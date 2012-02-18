/*
 * Copyright (c) 2010 Google Inc. All rights reserved.
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
#include "ScriptArguments.h"

#include "ScriptValue.h"

namespace WebCore {

PassRefPtr<ScriptArguments> ScriptArguments::create(ScriptState* scriptState, Vector<ScriptValue>& arguments)
{
    return adoptRef(new ScriptArguments(scriptState, arguments));
}

ScriptArguments::ScriptArguments(ScriptState* scriptState, Vector<ScriptValue>& arguments)
    : m_scriptState(scriptState)
{
    m_arguments.swap(arguments);
}

ScriptArguments::~ScriptArguments()
{
}

const ScriptValue &ScriptArguments::argumentAt(size_t index) const
{
    ASSERT(m_arguments.size() > index);
    return m_arguments[index];
}

ScriptState* ScriptArguments::globalState() const
{
    return m_scriptState.get();
}

bool ScriptArguments::getFirstArgumentAsString(String& result, bool checkForNullOrUndefined)
{
    if (!argumentCount())
        return false;

    const ScriptValue& value = argumentAt(0);
    if (checkForNullOrUndefined && (value.isNull() || value.isUndefined()))
        return false;

    if (!globalState()) {
        ASSERT_NOT_REACHED();
        return false;
    }

    result = value.toString(globalState());
    return true;
}

bool ScriptArguments::isEqual(ScriptArguments* other) const
{
    if (!other)
        return false;

    if (m_arguments.size() != other->m_arguments.size())
        return false;
    if (!globalState() && m_arguments.size())
        return false;

    for (size_t i = 0; i < m_arguments.size(); ++i) {
        if (!m_arguments[i].isEqual(other->globalState(), other->m_arguments[i]))
            return false;
    }
    return true;
}

} // namespace WebCore
