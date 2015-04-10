/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NPJSObject.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "JSNPObject.h"
#include "NPRuntimeObjectMap.h"
#include "NPRuntimeUtilities.h"
#include <JavaScriptCore/JSCJSValueInlines.h>
#include <JavaScriptCore/JSCellInlines.h>
#include <JavaScriptCore/JSLock.h>
#include <JavaScriptCore/JSObject.h>
#include <JavaScriptCore/StrongInlines.h>
#include <JavaScriptCore/StructureInlines.h>
#include <WebCore/Frame.h>
#include <WebCore/IdentifierRep.h>
#include <wtf/text/WTFString.h>

using namespace JSC;
using namespace WebCore;

namespace WebKit {

NPJSObject* NPJSObject::create(VM& vm, NPRuntimeObjectMap* objectMap, JSObject* jsObject)
{
    // We should never have a JSNPObject inside an NPJSObject.
    ASSERT(!jsObject->inherits(&JSNPObject::s_info));

    NPJSObject* npJSObject = toNPJSObject(createNPObject(0, npClass()));
    npJSObject->initialize(vm, objectMap, jsObject);

    return npJSObject;
}

NPJSObject::NPJSObject()
    : m_objectMap(0)
{
}

NPJSObject::~NPJSObject()
{
    m_objectMap->npJSObjectDestroyed(this);
}

bool NPJSObject::isNPJSObject(NPObject* npObject)
{
    return npObject->_class == npClass();
}

void NPJSObject::initialize(VM& vm, NPRuntimeObjectMap* objectMap, JSObject* jsObject)
{
    ASSERT(!m_objectMap);
    ASSERT(!m_jsObject);

    m_objectMap = objectMap;
    m_jsObject.set(vm, jsObject);
}

static Identifier identifierFromIdentifierRep(ExecState* exec, IdentifierRep* identifierRep)
{
    ASSERT(identifierRep->isString());

    const char* string = identifierRep->string();
    int length = strlen(string);

    return Identifier(exec, String::fromUTF8WithLatin1Fallback(string, length).impl());
}

bool NPJSObject::hasMethod(NPIdentifier methodName)
{
    IdentifierRep* identifierRep = static_cast<IdentifierRep*>(methodName);

    if (!identifierRep->isString())
        return false;

    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;

    JSLockHolder lock(exec);

    JSValue value = m_jsObject->get(exec, identifierFromIdentifierRep(exec, identifierRep));    
    exec->clearException();

    CallData callData;
    return getCallData(value, callData) != CallTypeNone;
}

bool NPJSObject::invoke(NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    IdentifierRep* identifierRep = static_cast<IdentifierRep*>(methodName);
    
    if (!identifierRep->isString())
        return false;
    
    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;
    
    JSLockHolder lock(exec);

    JSValue function = m_jsObject->get(exec, identifierFromIdentifierRep(exec, identifierRep));
    return invoke(exec, m_objectMap->globalObject(), function, arguments, argumentCount, result);
}

bool NPJSObject::invokeDefault(const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;

    JSLockHolder lock(exec);

    JSValue function = m_jsObject.get();
    return invoke(exec, m_objectMap->globalObject(), function, arguments, argumentCount, result);
}

bool NPJSObject::hasProperty(NPIdentifier identifier)
{
    IdentifierRep* identifierRep = static_cast<IdentifierRep*>(identifier);
    
    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;
    
    JSLockHolder lock(exec);

    bool result;
    if (identifierRep->isString())
        result = m_jsObject->hasProperty(exec, identifierFromIdentifierRep(exec, identifierRep));
    else
        result = m_jsObject->hasProperty(exec, identifierRep->number());

    exec->clearException();
    return result;
}

bool NPJSObject::getProperty(NPIdentifier propertyName, NPVariant* result)
{
    IdentifierRep* identifierRep = static_cast<IdentifierRep*>(propertyName);
    
    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;

    JSLockHolder lock(exec);
    JSValue jsResult;
    if (identifierRep->isString())
        jsResult = m_jsObject->get(exec, identifierFromIdentifierRep(exec, identifierRep));
    else
        jsResult = m_jsObject->get(exec, identifierRep->number());
    
    m_objectMap->convertJSValueToNPVariant(exec, jsResult, *result);
    exec->clearException();
    return true;
}

bool NPJSObject::setProperty(NPIdentifier propertyName, const NPVariant* value)
{
    IdentifierRep* identifierRep = static_cast<IdentifierRep*>(propertyName);
    
    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;
    
    JSLockHolder lock(exec);

    JSValue jsValue = m_objectMap->convertNPVariantToJSValue(exec, m_objectMap->globalObject(), *value);
    if (identifierRep->isString()) {
        PutPropertySlot slot;
        m_jsObject->methodTable()->put(m_jsObject.get(), exec, identifierFromIdentifierRep(exec, identifierRep), jsValue, slot);
    } else
        m_jsObject->methodTable()->putByIndex(m_jsObject.get(), exec, identifierRep->number(), jsValue, false);
    exec->clearException();
    
    return true;
}

bool NPJSObject::removeProperty(NPIdentifier propertyName)
{
    IdentifierRep* identifierRep = static_cast<IdentifierRep*>(propertyName);
    
    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;

    JSLockHolder lock(exec);
    if (identifierRep->isString()) {
        Identifier identifier = identifierFromIdentifierRep(exec, identifierRep);
        
        if (!m_jsObject->hasProperty(exec, identifier)) {
            exec->clearException();
            return false;
        }
        
        m_jsObject->methodTable()->deleteProperty(m_jsObject.get(), exec, identifier);
    } else {
        if (!m_jsObject->hasProperty(exec, identifierRep->number())) {
            exec->clearException();
            return false;
        }

        m_jsObject->methodTable()->deletePropertyByIndex(m_jsObject.get(), exec, identifierRep->number());
    }

    exec->clearException();
    return true;
}

bool NPJSObject::enumerate(NPIdentifier** identifiers, uint32_t* identifierCount)
{
    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;
    
    JSLockHolder lock(exec);

    PropertyNameArray propertyNames(exec);
    m_jsObject->methodTable()->getPropertyNames(m_jsObject.get(), exec, propertyNames, ExcludeDontEnumProperties);

    NPIdentifier* nameIdentifiers = npnMemNewArray<NPIdentifier>(propertyNames.size());

    for (size_t i = 0; i < propertyNames.size(); ++i)
        nameIdentifiers[i] = static_cast<NPIdentifier>(IdentifierRep::get(propertyNames[i].string().utf8().data()));

    *identifiers = nameIdentifiers;
    *identifierCount = propertyNames.size();

    return true;
}

bool NPJSObject::construct(const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    ExecState* exec = m_objectMap->globalExec();
    if (!exec)
        return false;

    JSLockHolder lock(exec);

    ConstructData constructData;
    ConstructType constructType = getConstructData(m_jsObject.get(), constructData);
    if (constructType == ConstructTypeNone)
        return false;

    // Convert the passed in arguments.
    MarkedArgumentBuffer argumentList;
    for (uint32_t i = 0; i < argumentCount; ++i)
        argumentList.append(m_objectMap->convertNPVariantToJSValue(exec, m_objectMap->globalObject(), arguments[i]));

    JSValue value = JSC::construct(exec, m_jsObject.get(), constructType, constructData, argumentList);
    
    // Convert and return the new object.
    m_objectMap->convertJSValueToNPVariant(exec, value, *result);
    exec->clearException();

    return true;
}

bool NPJSObject::invoke(ExecState* exec, JSGlobalObject* globalObject, JSValue function, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return false;

    // Convert the passed in arguments.
    MarkedArgumentBuffer argumentList;
    for (uint32_t i = 0; i < argumentCount; ++i)
        argumentList.append(m_objectMap->convertNPVariantToJSValue(exec, globalObject, arguments[i]));

    JSValue value = JSC::call(exec, function, callType, callData, m_jsObject->methodTable()->toThisObject(m_jsObject.get(), exec), argumentList);

    // Convert and return the result of the function call.
    m_objectMap->convertJSValueToNPVariant(exec, value, *result);
    exec->clearException();
    
    return true;
}

NPClass* NPJSObject::npClass()
{
    static NPClass npClass = {
        NP_CLASS_STRUCT_VERSION,
        NP_Allocate,
        NP_Deallocate,
        0,
        NP_HasMethod,
        NP_Invoke,
        NP_InvokeDefault,
        NP_HasProperty,
        NP_GetProperty,
        NP_SetProperty,
        NP_RemoveProperty,
        NP_Enumerate,
        NP_Construct
    };

    return &npClass;
}
    
NPObject* NPJSObject::NP_Allocate(NPP npp, NPClass*)
{
    ASSERT_UNUSED(npp, !npp);

    return new NPJSObject;
}

void NPJSObject::NP_Deallocate(NPObject* npObject)
{
    NPJSObject* npJSObject = toNPJSObject(npObject);
    delete npJSObject;
}

bool NPJSObject::NP_HasMethod(NPObject* npObject, NPIdentifier methodName)
{
    return toNPJSObject(npObject)->hasMethod(methodName);
}
    
bool NPJSObject::NP_Invoke(NPObject* npObject, NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    return toNPJSObject(npObject)->invoke(methodName, arguments, argumentCount, result);
}
    
bool NPJSObject::NP_InvokeDefault(NPObject* npObject, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    return toNPJSObject(npObject)->invokeDefault(arguments, argumentCount, result);
}
    
bool NPJSObject::NP_HasProperty(NPObject* npObject, NPIdentifier propertyName)
{
    return toNPJSObject(npObject)->hasProperty(propertyName);
}

bool NPJSObject::NP_GetProperty(NPObject* npObject, NPIdentifier propertyName, NPVariant* result)
{
    return toNPJSObject(npObject)->getProperty(propertyName, result);
}

bool NPJSObject::NP_SetProperty(NPObject* npObject, NPIdentifier propertyName, const NPVariant* value)
{
    return toNPJSObject(npObject)->setProperty(propertyName, value);
}

bool NPJSObject::NP_RemoveProperty(NPObject* npObject, NPIdentifier propertyName)
{
    return toNPJSObject(npObject)->removeProperty(propertyName);
}

bool NPJSObject::NP_Enumerate(NPObject* npObject, NPIdentifier** identifiers, uint32_t* identifierCount)
{
    return toNPJSObject(npObject)->enumerate(identifiers, identifierCount);
}

bool NPJSObject::NP_Construct(NPObject* npObject, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    return toNPJSObject(npObject)->construct(arguments, argumentCount, result);
}
    
} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
