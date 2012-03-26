/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Kelvin W Sherlock (ksherlock@gmail.com)
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
#include "JSObjectRef.h"
#include "JSObjectRefPrivate.h"

#include "APICast.h"
#include "CodeBlock.h"
#include "DateConstructor.h"
#include "ErrorConstructor.h"
#include "FunctionConstructor.h"
#include "Identifier.h"
#include "InitializeThreading.h"
#include "JSArray.h"
#include "JSCallbackConstructor.h"
#include "JSCallbackFunction.h"
#include "JSCallbackObject.h"
#include "JSClassRef.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "JSRetainPtr.h"
#include "JSString.h"
#include "JSValueRef.h"
#include "ObjectPrototype.h"
#include "PropertyNameArray.h"
#include "RegExpConstructor.h"

using namespace JSC;

JSClassRef JSClassCreate(const JSClassDefinition* definition)
{
    initializeThreading();
    RefPtr<OpaqueJSClass> jsClass = (definition->attributes & kJSClassAttributeNoAutomaticPrototype)
        ? OpaqueJSClass::createNoAutomaticPrototype(definition)
        : OpaqueJSClass::create(definition);
    
    return jsClass.release().leakRef();
}

JSClassRef JSClassRetain(JSClassRef jsClass)
{
    jsClass->ref();
    return jsClass;
}

void JSClassRelease(JSClassRef jsClass)
{
    jsClass->deref();
}

JSObjectRef JSObjectMake(JSContextRef ctx, JSClassRef jsClass, void* data)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    if (!jsClass)
        return toRef(constructEmptyObject(exec));

    JSCallbackObject<JSObjectWithGlobalObject>* object = new (exec) JSCallbackObject<JSObjectWithGlobalObject>(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->callbackObjectStructure(), jsClass, data);
    if (JSObject* prototype = jsClass->prototype(exec))
        object->setPrototype(exec->globalData(), prototype);

    return toRef(object);
}

JSObjectRef JSObjectMakeFunctionWithCallback(JSContextRef ctx, JSStringRef name, JSObjectCallAsFunctionCallback callAsFunction)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    Identifier nameID = name ? name->identifier(&exec->globalData()) : Identifier(exec, "anonymous");
    
    return toRef(new (exec) JSCallbackFunction(exec, exec->lexicalGlobalObject(), callAsFunction, nameID));
}

JSObjectRef JSObjectMakeConstructor(JSContextRef ctx, JSClassRef jsClass, JSObjectCallAsConstructorCallback callAsConstructor)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsPrototype = jsClass ? jsClass->prototype(exec) : 0;
    if (!jsPrototype)
        jsPrototype = exec->lexicalGlobalObject()->objectPrototype();

    JSCallbackConstructor* constructor = new (exec) JSCallbackConstructor(exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->callbackConstructorStructure(), jsClass, callAsConstructor);
    constructor->putDirect(exec->globalData(), exec->propertyNames().prototype, jsPrototype, DontEnum | DontDelete | ReadOnly);
    return toRef(constructor);
}

JSObjectRef JSObjectMakeFunction(JSContextRef ctx, JSStringRef name, unsigned parameterCount, const JSStringRef parameterNames[], JSStringRef body, JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    Identifier nameID = name ? name->identifier(&exec->globalData()) : Identifier(exec, "anonymous");
    
    MarkedArgumentBuffer args;
    for (unsigned i = 0; i < parameterCount; i++)
        args.append(jsString(exec, parameterNames[i]->ustring()));
    args.append(jsString(exec, body->ustring()));

    JSObject* result = constructFunction(exec, exec->lexicalGlobalObject(), args, nameID, sourceURL->ustring(), startingLineNumber);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }
    return toRef(result);
}

JSObjectRef JSObjectMakeArray(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[],  JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* result;
    if (argumentCount) {
        MarkedArgumentBuffer argList;
        for (size_t i = 0; i < argumentCount; ++i)
            argList.append(toJS(exec, arguments[i]));

        result = constructArray(exec, argList);
    } else
        result = constructEmptyArray(exec);

    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }

    return toRef(result);
}

JSObjectRef JSObjectMakeDate(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[],  JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; ++i)
        argList.append(toJS(exec, arguments[i]));

    JSObject* result = constructDate(exec, exec->lexicalGlobalObject(), argList);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }

    return toRef(result);
}

JSObjectRef JSObjectMakeError(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[],  JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue message = argumentCount ? toJS(exec, arguments[0]) : jsUndefined();
    Structure* errorStructure = exec->lexicalGlobalObject()->errorStructure();
    JSObject* result = ErrorInstance::create(exec, errorStructure, message);

    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }

    return toRef(result);
}

JSObjectRef JSObjectMakeRegExp(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[],  JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; ++i)
        argList.append(toJS(exec, arguments[i]));

    JSObject* result = constructRegExp(exec, exec->lexicalGlobalObject(),  argList);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }
    
    return toRef(result);
}

JSValueRef JSObjectGetPrototype(JSContextRef ctx, JSObjectRef object)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    return toRef(exec, jsObject->prototype());
}

void JSObjectSetPrototype(JSContextRef ctx, JSObjectRef object, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    JSValue jsValue = toJS(exec, value);

    jsObject->setPrototypeWithCycleCheck(exec->globalData(), jsValue.isObject() ? jsValue : jsNull());
}

bool JSObjectHasProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    
    return jsObject->hasProperty(exec, propertyName->identifier(&exec->globalData()));
}

JSValueRef JSObjectGetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);

    JSValue jsValue = jsObject->get(exec, propertyName->identifier(&exec->globalData()));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return toRef(exec, jsValue);
}

void JSObjectSetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSPropertyAttributes attributes, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    Identifier name(propertyName->identifier(&exec->globalData()));
    JSValue jsValue = toJS(exec, value);

    if (attributes && !jsObject->hasProperty(exec, name))
        jsObject->putWithAttributes(exec, name, jsValue, attributes);
    else {
        PutPropertySlot slot;
        jsObject->put(exec, name, jsValue, slot);
    }

    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
}

JSValueRef JSObjectGetPropertyAtIndex(JSContextRef ctx, JSObjectRef object, unsigned propertyIndex, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);

    JSValue jsValue = jsObject->get(exec, propertyIndex);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return toRef(exec, jsValue);
}


void JSObjectSetPropertyAtIndex(JSContextRef ctx, JSObjectRef object, unsigned propertyIndex, JSValueRef value, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    JSValue jsValue = toJS(exec, value);
    
    jsObject->put(exec, propertyIndex, jsValue);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
}

bool JSObjectDeleteProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);

    bool result = jsObject->deleteProperty(exec, propertyName->identifier(&exec->globalData()));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return result;
}

void* JSObjectGetPrivate(JSObjectRef object)
{
    JSObject* jsObject = toJS(object);
    
    if (jsObject->inherits(&JSCallbackObject<JSGlobalObject>::s_info))
        return static_cast<JSCallbackObject<JSGlobalObject>*>(jsObject)->getPrivate();
    if (jsObject->inherits(&JSCallbackObject<JSObjectWithGlobalObject>::s_info))
        return static_cast<JSCallbackObject<JSObjectWithGlobalObject>*>(jsObject)->getPrivate();
    
    return 0;
}

bool JSObjectSetPrivate(JSObjectRef object, void* data)
{
    JSObject* jsObject = toJS(object);
    
    if (jsObject->inherits(&JSCallbackObject<JSGlobalObject>::s_info)) {
        static_cast<JSCallbackObject<JSGlobalObject>*>(jsObject)->setPrivate(data);
        return true;
    }
    if (jsObject->inherits(&JSCallbackObject<JSObjectWithGlobalObject>::s_info)) {
        static_cast<JSCallbackObject<JSObjectWithGlobalObject>*>(jsObject)->setPrivate(data);
        return true;
    }
        
    return false;
}

JSValueRef JSObjectGetPrivateProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    JSObject* jsObject = toJS(object);
    JSValue result;
    Identifier name(propertyName->identifier(&exec->globalData()));
    if (jsObject->inherits(&JSCallbackObject<JSGlobalObject>::s_info))
        result = static_cast<JSCallbackObject<JSGlobalObject>*>(jsObject)->getPrivateProperty(name);
    else if (jsObject->inherits(&JSCallbackObject<JSObjectWithGlobalObject>::s_info))
        result = static_cast<JSCallbackObject<JSObjectWithGlobalObject>*>(jsObject)->getPrivateProperty(name);
    return toRef(exec, result);
}

bool JSObjectSetPrivateProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    JSObject* jsObject = toJS(object);
    JSValue jsValue = value ? toJS(exec, value) : JSValue();
    Identifier name(propertyName->identifier(&exec->globalData()));
    if (jsObject->inherits(&JSCallbackObject<JSGlobalObject>::s_info)) {
        static_cast<JSCallbackObject<JSGlobalObject>*>(jsObject)->setPrivateProperty(exec->globalData(), name, jsValue);
        return true;
    }
    if (jsObject->inherits(&JSCallbackObject<JSObjectWithGlobalObject>::s_info)) {
        static_cast<JSCallbackObject<JSObjectWithGlobalObject>*>(jsObject)->setPrivateProperty(exec->globalData(), name, jsValue);
        return true;
    }
    return false;
}

bool JSObjectDeletePrivateProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    JSObject* jsObject = toJS(object);
    Identifier name(propertyName->identifier(&exec->globalData()));
    if (jsObject->inherits(&JSCallbackObject<JSGlobalObject>::s_info)) {
        static_cast<JSCallbackObject<JSGlobalObject>*>(jsObject)->deletePrivateProperty(name);
        return true;
    }
    if (jsObject->inherits(&JSCallbackObject<JSObjectWithGlobalObject>::s_info)) {
        static_cast<JSCallbackObject<JSObjectWithGlobalObject>*>(jsObject)->deletePrivateProperty(name);
        return true;
    }
    return false;
}

bool JSObjectIsFunction(JSContextRef, JSObjectRef object)
{
    CallData callData;
    return toJS(object)->getCallData(callData) != CallTypeNone;
}

JSValueRef JSObjectCallAsFunction(JSContextRef ctx, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    JSObject* jsThisObject = toJS(thisObject);

    if (!jsThisObject)
        jsThisObject = exec->globalThisValue();

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; i++)
        argList.append(toJS(exec, arguments[i]));

    CallData callData;
    CallType callType = jsObject->getCallData(callData);
    if (callType == CallTypeNone)
        return 0;

    JSValueRef result = toRef(exec, call(exec, jsObject, callType, callData, jsThisObject, argList));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }
    return result;
}

bool JSObjectIsConstructor(JSContextRef, JSObjectRef object)
{
    JSObject* jsObject = toJS(object);
    ConstructData constructData;
    return jsObject->getConstructData(constructData) != ConstructTypeNone;
}

JSObjectRef JSObjectCallAsConstructor(JSContextRef ctx, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);

    ConstructData constructData;
    ConstructType constructType = jsObject->getConstructData(constructData);
    if (constructType == ConstructTypeNone)
        return 0;

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; i++)
        argList.append(toJS(exec, arguments[i]));
    JSObjectRef result = toRef(construct(exec, jsObject, constructType, constructData, argList));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }
    return result;
}

struct OpaqueJSPropertyNameArray {
    WTF_MAKE_FAST_ALLOCATED;
public:
    OpaqueJSPropertyNameArray(JSGlobalData* globalData)
        : refCount(0)
        , globalData(globalData)
    {
    }
    
    unsigned refCount;
    JSGlobalData* globalData;
    Vector<JSRetainPtr<JSStringRef> > array;
};

JSPropertyNameArrayRef JSObjectCopyPropertyNames(JSContextRef ctx, JSObjectRef object)
{
    JSObject* jsObject = toJS(object);
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSGlobalData* globalData = &exec->globalData();

    JSPropertyNameArrayRef propertyNames = new OpaqueJSPropertyNameArray(globalData);
    PropertyNameArray array(globalData);
    jsObject->getPropertyNames(exec, array);

    size_t size = array.size();
    propertyNames->array.reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i)
        propertyNames->array.append(JSRetainPtr<JSStringRef>(Adopt, OpaqueJSString::create(array[i].ustring()).leakRef()));
    
    return JSPropertyNameArrayRetain(propertyNames);
}

JSPropertyNameArrayRef JSPropertyNameArrayRetain(JSPropertyNameArrayRef array)
{
    ++array->refCount;
    return array;
}

void JSPropertyNameArrayRelease(JSPropertyNameArrayRef array)
{
    if (--array->refCount == 0) {
        APIEntryShim entryShim(array->globalData, false);
        delete array;
    }
}

size_t JSPropertyNameArrayGetCount(JSPropertyNameArrayRef array)
{
    return array->array.size();
}

JSStringRef JSPropertyNameArrayGetNameAtIndex(JSPropertyNameArrayRef array, size_t index)
{
    return array->array[static_cast<unsigned>(index)].get();
}

void JSPropertyNameAccumulatorAddName(JSPropertyNameAccumulatorRef array, JSStringRef propertyName)
{
    PropertyNameArray* propertyNames = toJS(array);
    APIEntryShim entryShim(propertyNames->globalData());
    propertyNames->add(propertyName->identifier(propertyNames->globalData()));
}
