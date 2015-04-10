/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Michael Pruett <michael@68k.org>
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

#if ENABLE(INDEXED_DATABASE)
#include "IDBBindingUtilities.h"

#include "DOMRequestState.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBTracing.h"
#include "SharedBuffer.h"

#include <runtime/DateInstance.h>
#include <runtime/ObjectConstructor.h>

using namespace JSC;

namespace WebCore {

static bool get(ExecState* exec, JSValue object, const String& keyPathElement, JSValue& result)
{
    if (object.isString() && keyPathElement == "length") {
        result = jsNumber(object.toString(exec)->length());
        return true;
    }
    if (!object.isObject())
        return false;
    Identifier identifier(&exec->vm(), keyPathElement.utf8().data());
    if (!asObject(object)->hasProperty(exec, identifier))
        return false;
    result = asObject(object)->get(exec, identifier);
    return true;
}

static bool canSet(JSValue object, const String& keyPathElement)
{
    UNUSED_PARAM(keyPathElement);
    return object.isObject();
}

static bool set(ExecState* exec, JSValue& object, const String& keyPathElement, JSValue jsValue)
{
    if (!canSet(object, keyPathElement))
        return false;
    Identifier identifier(&exec->vm(), keyPathElement.utf8().data());
    asObject(object)->putDirect(exec->vm(), identifier, jsValue);
    return true;
}

static JSValue idbKeyToJSValue(ExecState* exec, JSDOMGlobalObject* globalObject, IDBKey* key)
{
    if (!key) {
        // This should be undefined, not null.
        // Spec: http://dvcs.w3.org/hg/IndexedDB/raw-file/tip/Overview.html#idl-def-IDBKeyRange
        return jsUndefined();
    }

    switch (key->type()) {
    case IDBKey::ArrayType:
        {
            const IDBKey::KeyArray& inArray = key->array();
            size_t size = inArray.size();
            JSArray* outArray = constructEmptyArray(exec, 0, globalObject, size);
            for (size_t i = 0; i < size; ++i) {
                IDBKey* arrayKey = inArray.at(i).get();
                outArray->putDirectIndex(exec, i, idbKeyToJSValue(exec, globalObject, arrayKey));
            }
            return JSValue(outArray);
        }
    case IDBKey::StringType:
        return jsStringWithCache(exec, key->string());
    case IDBKey::DateType:
        return jsDateOrNull(exec, key->date());
    case IDBKey::NumberType:
        return jsNumber(key->number());
    case IDBKey::MinType:
    case IDBKey::InvalidType:
        ASSERT_NOT_REACHED();
        return jsUndefined();
    }

    ASSERT_NOT_REACHED();
    return jsUndefined();
}

static const size_t maximumDepth = 2000;

static PassRefPtr<IDBKey> createIDBKeyFromValue(ExecState* exec, JSValue value, Vector<JSArray*>& stack)
{
    if (value.isNumber() && !std::isnan(value.toNumber(exec)))
        return IDBKey::createNumber(value.toNumber(exec));
    if (value.isString())
        return IDBKey::createString(value.toString(exec)->value(exec));
    if (value.inherits(&DateInstance::s_info) && !std::isnan(valueToDate(exec, value)))
        return IDBKey::createDate(valueToDate(exec, value));
    if (value.isObject()) {
        JSObject* object = asObject(value);
        if (isJSArray(object) || object->inherits(&JSArray::s_info)) {
            JSArray* array = asArray(object);
            size_t length = array->length();

            if (stack.contains(array))
                return 0;
            if (stack.size() >= maximumDepth)
                return 0;
            stack.append(array);

            IDBKey::KeyArray subkeys;
            for (size_t i = 0; i < length; i++) {
                JSValue item = array->getIndex(exec, i);
                RefPtr<IDBKey> subkey = createIDBKeyFromValue(exec, item, stack);
                if (!subkey)
                    subkeys.append(IDBKey::createInvalid());
                else
                    subkeys.append(subkey);
            }

            stack.removeLast();
            return IDBKey::createArray(subkeys);
        }
    }
    return 0;
}

PassRefPtr<IDBKey> createIDBKeyFromValue(ExecState* exec, JSValue value)
{
    Vector<JSArray*> stack;
    RefPtr<IDBKey> key = createIDBKeyFromValue(exec, value, stack);
    if (key)
        return key;
    return IDBKey::createInvalid();
}

IDBKeyPath idbKeyPathFromValue(ExecState* exec, JSValue keyPathValue)
{
    IDBKeyPath keyPath;
    if (isJSArray(keyPathValue))
        keyPath = IDBKeyPath(toNativeArray<String>(exec, keyPathValue));
    else
        keyPath = IDBKeyPath(keyPathValue.toString(exec)->value(exec));
    return keyPath;
}

static JSValue getNthValueOnKeyPath(ExecState* exec, JSValue rootValue, const Vector<String>& keyPathElements, size_t index)
{
    JSValue currentValue(rootValue);
    ASSERT(index <= keyPathElements.size());
    for (size_t i = 0; i < index; i++) {
        JSValue parentValue(currentValue);
        if (!get(exec, parentValue, keyPathElements[i], currentValue))
            return jsUndefined();
    }
    return currentValue;
}

static PassRefPtr<IDBKey> createIDBKeyFromScriptValueAndKeyPath(ExecState* exec, const ScriptValue& value, const String& keyPath)
{
    Vector<String> keyPathElements;
    IDBKeyPathParseError error;
    IDBParseKeyPath(keyPath, keyPathElements, error);
    ASSERT(error == IDBKeyPathParseErrorNone);

    JSValue jsValue = value.jsValue();
    jsValue = getNthValueOnKeyPath(exec, jsValue, keyPathElements, keyPathElements.size());
    if (jsValue.isUndefined())
        return 0;
    return createIDBKeyFromValue(exec, jsValue);
}

static JSValue ensureNthValueOnKeyPath(ExecState* exec, JSValue rootValue, const Vector<String>& keyPathElements, size_t index)
{
    JSValue currentValue(rootValue);

    ASSERT(index <= keyPathElements.size());
    for (size_t i = 0; i < index; i++) {
        JSValue parentValue(currentValue);
        const String& keyPathElement = keyPathElements[i];
        if (!get(exec, parentValue, keyPathElement, currentValue)) {
            JSObject* object = constructEmptyObject(exec);
            if (!set(exec, parentValue, keyPathElement, JSValue(object)))
                return jsUndefined();
            currentValue = JSValue(object);
        }
    }

    return currentValue;
}

static bool canInjectNthValueOnKeyPath(ExecState* exec, JSValue rootValue, const Vector<String>& keyPathElements, size_t index)
{
    if (!rootValue.isObject())
        return false;

    JSValue currentValue(rootValue);

    ASSERT(index <= keyPathElements.size());
    for (size_t i = 0; i < index; ++i) {
        JSValue parentValue(currentValue);
        const String& keyPathElement = keyPathElements[i];
        if (!get(exec, parentValue, keyPathElement, currentValue))
            return canSet(parentValue, keyPathElement);
    }
    return true;
}

bool injectIDBKeyIntoScriptValue(DOMRequestState* requestState, PassRefPtr<IDBKey> key, ScriptValue& value, const IDBKeyPath& keyPath)
{
    IDB_TRACE("injectIDBKeyIntoScriptValue");

    ASSERT(keyPath.type() == IDBKeyPath::StringType);

    Vector<String> keyPathElements;
    IDBKeyPathParseError error;
    IDBParseKeyPath(keyPath.string(), keyPathElements, error);
    ASSERT(error == IDBKeyPathParseErrorNone);

    if (keyPathElements.isEmpty())
        return false;

    ExecState* exec = requestState->exec();

    JSValue parent = ensureNthValueOnKeyPath(exec, value.jsValue(), keyPathElements, keyPathElements.size() - 1);
    if (parent.isUndefined())
        return false;

    if (!set(exec, parent, keyPathElements.last(), idbKeyToJSValue(exec, jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), key.get())))
        return false;

    return true;
}

PassRefPtr<IDBKey> createIDBKeyFromScriptValueAndKeyPath(DOMRequestState* requestState, const ScriptValue& value, const IDBKeyPath& keyPath)
{
    IDB_TRACE("createIDBKeyFromScriptValueAndKeyPath");
    ASSERT(!keyPath.isNull());

    ExecState* exec = requestState->exec();

    if (keyPath.type() == IDBKeyPath::ArrayType) {
        IDBKey::KeyArray result;
        const Vector<String>& array = keyPath.array();
        for (size_t i = 0; i < array.size(); i++) {
            RefPtr<IDBKey> key = createIDBKeyFromScriptValueAndKeyPath(exec, value, array[i]);
            if (!key)
                return 0;
            result.append(key);
        }
        return IDBKey::createArray(result);
    }

    ASSERT(keyPath.type() == IDBKeyPath::StringType);
    return createIDBKeyFromScriptValueAndKeyPath(exec, value, keyPath.string());
}

bool canInjectIDBKeyIntoScriptValue(DOMRequestState* requestState, const ScriptValue& scriptValue, const IDBKeyPath& keyPath)
{
    IDB_TRACE("canInjectIDBKeyIntoScriptValue");

    ASSERT(keyPath.type() == IDBKeyPath::StringType);
    Vector<String> keyPathElements;
    IDBKeyPathParseError error;
    IDBParseKeyPath(keyPath.string(), keyPathElements, error);
    ASSERT(error == IDBKeyPathParseErrorNone);

    if (!keyPathElements.size())
        return false;

    JSC::ExecState* exec = requestState->exec();
    return canInjectNthValueOnKeyPath(exec, scriptValue.jsValue(), keyPathElements, keyPathElements.size() - 1);
}

ScriptValue deserializeIDBValue(DOMRequestState* requestState, PassRefPtr<SerializedScriptValue> prpValue)
{
    ExecState* exec = requestState->exec();
    RefPtr<SerializedScriptValue> serializedValue = prpValue;
    if (serializedValue)
        return ScriptValue::deserialize(exec, serializedValue.get(), NonThrowing);
    return ScriptValue(exec->vm(), jsNull());
}

ScriptValue deserializeIDBValueBuffer(DOMRequestState* requestState, PassRefPtr<SharedBuffer> prpBuffer)
{
    ExecState* exec = requestState->exec();
    RefPtr<SharedBuffer> buffer = prpBuffer;
    if (buffer) {
        // FIXME: The extra copy here can be eliminated by allowing SerializedScriptValue to take a raw const char* or const uint8_t*.
        Vector<uint8_t> value;
        value.append(buffer->data(), buffer->size());
        RefPtr<SerializedScriptValue> serializedValue = SerializedScriptValue::createFromWireBytes(value);
        return ScriptValue::deserialize(exec, serializedValue.get(), NonThrowing);
    }
    return ScriptValue(exec->vm(), jsNull());
}

ScriptValue idbKeyToScriptValue(DOMRequestState* requestState, PassRefPtr<IDBKey> key)
{
    ExecState* exec = requestState->exec();
    return ScriptValue(exec->vm(), idbKeyToJSValue(exec, jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), key.get()));
}

PassRefPtr<IDBKey> scriptValueToIDBKey(DOMRequestState* requestState, const ScriptValue& scriptValue)
{
    ExecState* exec = requestState->exec();
    return createIDBKeyFromValue(exec, scriptValue.jsValue());
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
