/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Dictionary.h"

#if ENABLE(NOTIFICATIONS)
#include "JSNotification.h"
#include "Notification.h"
#endif

using namespace JSC;

namespace WebCore {

Dictionary::Dictionary()
    : m_dictionary(0, 0)
{
}

Dictionary::Dictionary(JSC::ExecState* exec, JSC::JSValue value)
    : m_dictionary(exec, value.isObject() ? value.getObject() : 0)
{
}


#if ENABLE(NOTIFICATIONS)
template<>
JSObject* Dictionary::asJSObject<Notification>(Notification* object) const
{
    return asObject(toJS(m_dictionary.execState(), jsCast<JSDOMGlobalObject*>(m_dictionary.execState()->lexicalGlobalObject()), object));
}
#endif

bool Dictionary::getOwnPropertiesAsStringHashMap(HashMap<String, String>& map) const
{
    if (!m_dictionary.isValid())
        return false;

    JSObject* object =  m_dictionary.initializerObject();
    ExecState* exec = m_dictionary.execState();

    PropertyNameArray propertyNames(exec);
    JSObject::getOwnPropertyNames(object, exec, propertyNames, ExcludeDontEnumProperties);
    for (PropertyNameArray::const_iterator it = propertyNames.begin(); it != propertyNames.end(); ++it) {
        String stringKey = it->string();
        if (stringKey.isEmpty())
            continue;
        JSValue value = object->get(exec, *it);
        if (exec->hadException())
            continue;
        String stringValue = value.toString(exec)->value(exec);
        if (!exec->hadException())
            map.set(stringKey, stringValue);
    }

    return true;
}

bool Dictionary::getOwnPropertyNames(Vector<String>& names) const
{
    if (!m_dictionary.isValid())
        return false;

    JSObject* object =  m_dictionary.initializerObject();
    ExecState* exec = m_dictionary.execState();

    PropertyNameArray propertyNames(exec);
    JSObject::getOwnPropertyNames(object, exec, propertyNames, ExcludeDontEnumProperties);
    for (PropertyNameArray::const_iterator it = propertyNames.begin(); it != propertyNames.end(); ++it) {
        String stringKey = it->string();
        if (!stringKey.isEmpty())
            names.append(stringKey);
    }

    return true;
}

bool Dictionary::getWithUndefinedOrNullCheck(const String& propertyName, String& value) const
{
    return m_dictionary.getWithUndefinedOrNullCheck(propertyName, value);
}

};
