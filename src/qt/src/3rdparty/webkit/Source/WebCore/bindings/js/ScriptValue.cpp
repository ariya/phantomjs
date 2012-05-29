/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (c) 2011 Google Inc. All rights reserved.
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
#include "ScriptValue.h"

#include "InspectorValues.h"
#include "SerializedScriptValue.h"

#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSValueRef.h>

#include <heap/Strong.h>
#include <runtime/JSLock.h>
#include <runtime/UString.h>

using namespace JSC;

namespace WebCore {

bool ScriptValue::getString(ScriptState* scriptState, String& result) const
{
    if (!m_value)
        return false;
    JSLock lock(SilenceAssertionsOnly);
    UString ustring;
    if (!m_value.get().getString(scriptState, ustring))
        return false;
    result = ustringToString(ustring);
    return true;
}

String ScriptValue::toString(ScriptState* scriptState) const
{
    String result = ustringToString(m_value.get().toString(scriptState));
    // Handle the case where an exception is thrown as part of invoking toString on the object.
    if (scriptState->hadException())
        scriptState->clearException();
    return result;
}

bool ScriptValue::isEqual(ScriptState* scriptState, const ScriptValue& anotherValue) const
{
    if (hasNoValue())
        return anotherValue.hasNoValue();

    return JSValueIsEqual(toRef(scriptState), toRef(scriptState, jsValue()), toRef(scriptState, anotherValue.jsValue()), 0);
}

bool ScriptValue::isNull() const
{
    if (!m_value)
        return false;
    return m_value.get().isNull();
}

bool ScriptValue::isUndefined() const
{
    if (!m_value)
        return false;
    return m_value.get().isUndefined();
}

bool ScriptValue::isObject() const
{
    if (!m_value)
        return false;
    return m_value.get().isObject();
}

bool ScriptValue::isFunction() const
{
    CallData callData;
    return getCallData(m_value.get(), callData) != CallTypeNone;
}

PassRefPtr<SerializedScriptValue> ScriptValue::serialize(ScriptState* scriptState, SerializationErrorMode throwExceptions)
{
    return SerializedScriptValue::create(scriptState, jsValue(), throwExceptions);
}

ScriptValue ScriptValue::deserialize(ScriptState* scriptState, SerializedScriptValue* value, SerializationErrorMode throwExceptions)
{
    return ScriptValue(scriptState->globalData(), value->deserialize(scriptState, scriptState->lexicalGlobalObject(), throwExceptions));
}

#if ENABLE(INSPECTOR)
static PassRefPtr<InspectorValue> jsToInspectorValue(ScriptState* scriptState, JSValue value)
{
    if (!value) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    if (value.isNull() || value.isUndefined())
        return InspectorValue::null();
    if (value.isBoolean())
        return InspectorBasicValue::create(value.getBoolean());
    if (value.isNumber())
        return InspectorBasicValue::create(value.uncheckedGetNumber());
    if (value.isString()) {
        UString s = value.getString(scriptState);
        return InspectorString::create(String(s.characters(), s.length()));
    }
    if (value.isObject()) {
        if (isJSArray(&scriptState->globalData(), value)) {
            RefPtr<InspectorArray> inspectorArray = InspectorArray::create();
            JSArray* array = asArray(value);
            unsigned length = array->length();
            for (unsigned i = 0; i < length; i++) {
                JSValue element = array->getIndex(i);
                RefPtr<InspectorValue> elementValue = jsToInspectorValue(scriptState, element);
                if (!elementValue) {
                    ASSERT_NOT_REACHED();
                    elementValue = InspectorValue::null();
                }
                inspectorArray->pushValue(elementValue);
            }
            return inspectorArray;
        }
        RefPtr<InspectorObject> inspectorObject = InspectorObject::create();
        JSObject* object = value.getObject();
        PropertyNameArray propertyNames(scriptState);
        object->getOwnPropertyNames(scriptState, propertyNames);
        for (size_t i = 0; i < propertyNames.size(); i++) {
            const Identifier& name =  propertyNames[i];
            JSValue propertyValue = object->get(scriptState, name);
            RefPtr<InspectorValue> inspectorValue = jsToInspectorValue(scriptState, propertyValue);
            if (!inspectorValue) {
                ASSERT_NOT_REACHED();
                inspectorValue = InspectorValue::null();
            }
            inspectorObject->setValue(String(name.characters(), name.length()), inspectorValue);
        }
        return inspectorObject;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

PassRefPtr<InspectorValue> ScriptValue::toInspectorValue(ScriptState* scriptState) const
{
    return jsToInspectorValue(scriptState, m_value.get());
}
#endif // ENABLE(INSPECTOR)

} // namespace WebCore
