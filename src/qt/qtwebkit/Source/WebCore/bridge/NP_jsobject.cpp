/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "NP_jsobject.h"

#include "PluginView.h"
#include "c_utility.h"
#include "c_instance.h"
#include "IdentifierRep.h"
#include "JSDOMBinding.h"
#include "npruntime_impl.h"
#include "npruntime_priv.h"
#include "runtime_root.h"
#include <runtime/Error.h>
#include <runtime/JSGlobalObject.h>
#include <runtime/JSLock.h>
#include <runtime/PropertyNameArray.h>
#include <parser/SourceCode.h>
#include <runtime/Completion.h>
#include <wtf/text/WTFString.h>

using namespace JSC;
using namespace JSC::Bindings;
using namespace WebCore;

class ObjectMap {
public:
    NPObject* get(RootObject* rootObject, JSObject* jsObject)
    {
        return m_map.get(rootObject).get(jsObject);
    }

    void add(RootObject* rootObject, JSObject* jsObject, NPObject* npObject)
    {
        HashMap<RootObject*, JSToNPObjectMap>::iterator iter = m_map.find(rootObject);
        if (iter == m_map.end()) {
            rootObject->addInvalidationCallback(&m_invalidationCallback);
            iter = m_map.add(rootObject, JSToNPObjectMap()).iterator;
        }

        ASSERT(iter->value.find(jsObject) == iter->value.end());
        iter->value.add(jsObject, npObject);
    }

    void remove(RootObject* rootObject)
    {
        HashMap<RootObject*, JSToNPObjectMap>::iterator iter = m_map.find(rootObject);
        ASSERT(iter != m_map.end());
        m_map.remove(iter);
    }

    void remove(RootObject* rootObject, JSObject* jsObject)
    {
        HashMap<RootObject*, JSToNPObjectMap>::iterator iter = m_map.find(rootObject);
        ASSERT(iter != m_map.end());
        ASSERT(iter->value.find(jsObject) != iter->value.end());

        iter->value.remove(jsObject);
    }

private:
    struct RootObjectInvalidationCallback : public RootObject::InvalidationCallback {
        virtual void operator()(RootObject*);
    };
    RootObjectInvalidationCallback m_invalidationCallback;

    // JSObjects are protected by RootObject.
    typedef HashMap<JSObject*, NPObject*> JSToNPObjectMap;
    HashMap<RootObject*, JSToNPObjectMap> m_map;
};


static ObjectMap& objectMap()
{
    DEFINE_STATIC_LOCAL(ObjectMap, map, ());
    return map;
}

void ObjectMap::RootObjectInvalidationCallback::operator()(RootObject* rootObject)
{
    objectMap().remove(rootObject);
}

static void getListFromVariantArgs(ExecState* exec, const NPVariant* args, unsigned argCount, RootObject* rootObject, MarkedArgumentBuffer& aList)
{
    for (unsigned i = 0; i < argCount; ++i)
        aList.append(convertNPVariantToValue(exec, &args[i], rootObject));
}

static NPObject* jsAllocate(NPP, NPClass*)
{
    return static_cast<NPObject*>(malloc(sizeof(JavaScriptObject)));
}

static void jsDeallocate(NPObject* npObj)
{
    JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(npObj);

    if (obj->rootObject && obj->rootObject->isValid()) {
        objectMap().remove(obj->rootObject, obj->imp);
        obj->rootObject->gcUnprotect(obj->imp);
    }

    if (obj->rootObject)
        obj->rootObject->deref();

    free(obj);
}

static NPClass javascriptClass = { 1, jsAllocate, jsDeallocate, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static NPClass noScriptClass = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

NPClass* NPScriptObjectClass = &javascriptClass;
static NPClass* NPNoScriptObjectClass = &noScriptClass;

NPObject* _NPN_CreateScriptObject(NPP npp, JSObject* imp, PassRefPtr<RootObject> rootObject)
{
    if (NPObject* object = objectMap().get(rootObject.get(), imp))
        return _NPN_RetainObject(object);

    JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(_NPN_CreateObject(npp, NPScriptObjectClass));

    obj->rootObject = rootObject.leakRef();

    if (obj->rootObject) {
        obj->rootObject->gcProtect(imp);
        objectMap().add(obj->rootObject, imp, reinterpret_cast<NPObject*>(obj));
    }

    obj->imp = imp;

    return reinterpret_cast<NPObject*>(obj);
}

NPObject* _NPN_CreateNoScriptObject(void)
{
    return _NPN_CreateObject(0, NPNoScriptObjectClass);
}

bool _NPN_InvokeDefault(NPP, NPObject* o, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 
        
        VOID_TO_NPVARIANT(*result);
        
        // Lookup the function object.
        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;
        
        ExecState* exec = rootObject->globalObject()->globalExec();
        JSLockHolder lock(exec);
        
        // Call the function object.
        JSValue function = obj->imp;
        CallData callData;
        CallType callType = getCallData(function, callData);
        if (callType == CallTypeNone)
            return false;
        
        MarkedArgumentBuffer argList;
        getListFromVariantArgs(exec, args, argCount, rootObject, argList);
        JSValue resultV = JSC::call(exec, function, callType, callData, function, argList);

        // Convert and return the result of the function call.
        convertValueToNPVariant(exec, resultV, result);
        exec->clearException();
        return true;        
    }

    if (o->_class->invokeDefault)
        return o->_class->invokeDefault(o, args, argCount, result);    
    VOID_TO_NPVARIANT(*result);
    return true;
}

bool _NPN_Invoke(NPP npp, NPObject* o, NPIdentifier methodName, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 

        IdentifierRep* i = static_cast<IdentifierRep*>(methodName);
        if (!i->isString())
            return false;

        // Special case the "eval" method.
        if (methodName == _NPN_GetStringIdentifier("eval")) {
            if (argCount != 1)
                return false;
            if (args[0].type != NPVariantType_String)
                return false;
            return _NPN_Evaluate(npp, o, const_cast<NPString*>(&args[0].value.stringValue), result);
        }

        // Look up the function object.
        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;
        ExecState* exec = rootObject->globalObject()->globalExec();
        JSLockHolder lock(exec);
        JSValue function = obj->imp->get(exec, identifierFromNPIdentifier(exec, i->string()));
        CallData callData;
        CallType callType = getCallData(function, callData);
        if (callType == CallTypeNone)
            return false;

        // Call the function object.
        MarkedArgumentBuffer argList;
        getListFromVariantArgs(exec, args, argCount, rootObject, argList);
        JSValue resultV = JSC::call(exec, function, callType, callData, obj->imp->methodTable()->toThisObject(obj->imp, exec), argList);

        // Convert and return the result of the function call.
        convertValueToNPVariant(exec, resultV, result);
        exec->clearException();
        return true;
    }

    if (o->_class->invoke)
        return o->_class->invoke(o, methodName, args, argCount, result);
    
    VOID_TO_NPVARIANT(*result);
    return true;
}

bool _NPN_Evaluate(NPP instance, NPObject* o, NPString* s, NPVariant* variant)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 

        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;

        // There is a crash in Flash when evaluating a script that destroys the
        // PluginView, so we destroy it asynchronously.
        PluginView::keepAlive(instance);

        ExecState* exec = rootObject->globalObject()->globalExec();
        JSLockHolder lock(exec);
        String scriptString = convertNPStringToUTF16(s);
        
        JSValue returnValue = JSC::evaluate(rootObject->globalObject()->globalExec(), makeSource(scriptString), JSC::JSValue());

        convertValueToNPVariant(exec, returnValue, variant);
        exec->clearException();
        return true;
    }

    VOID_TO_NPVARIANT(*variant);
    return false;
}

bool _NPN_GetProperty(NPP, NPObject* o, NPIdentifier propertyName, NPVariant* variant)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 

        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;

        ExecState* exec = rootObject->globalObject()->globalExec();
        IdentifierRep* i = static_cast<IdentifierRep*>(propertyName);
        
        JSLockHolder lock(exec);
        JSValue result;
        if (i->isString())
            result = obj->imp->get(exec, identifierFromNPIdentifier(exec, i->string()));
        else
            result = obj->imp->get(exec, i->number());

        convertValueToNPVariant(exec, result, variant);
        exec->clearException();
        return true;
    }

    if (o->_class->hasProperty && o->_class->getProperty) {
        if (o->_class->hasProperty(o, propertyName))
            return o->_class->getProperty(o, propertyName, variant);
        return false;
    }

    VOID_TO_NPVARIANT(*variant);
    return false;
}

bool _NPN_SetProperty(NPP, NPObject* o, NPIdentifier propertyName, const NPVariant* variant)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 

        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;

        ExecState* exec = rootObject->globalObject()->globalExec();
        JSLockHolder lock(exec);
        IdentifierRep* i = static_cast<IdentifierRep*>(propertyName);

        if (i->isString()) {
            PutPropertySlot slot;
            obj->imp->methodTable()->put(obj->imp, exec, identifierFromNPIdentifier(exec, i->string()), convertNPVariantToValue(exec, variant, rootObject), slot);
        } else
            obj->imp->methodTable()->putByIndex(obj->imp, exec, i->number(), convertNPVariantToValue(exec, variant, rootObject), false);
        exec->clearException();
        return true;
    }

    if (o->_class->setProperty)
        return o->_class->setProperty(o, propertyName, variant);

    return false;
}

bool _NPN_RemoveProperty(NPP, NPObject* o, NPIdentifier propertyName)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 

        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;

        ExecState* exec = rootObject->globalObject()->globalExec();
        IdentifierRep* i = static_cast<IdentifierRep*>(propertyName);
        if (i->isString()) {
            if (!obj->imp->hasProperty(exec, identifierFromNPIdentifier(exec, i->string()))) {
                exec->clearException();
                return false;
            }
        } else {
            if (!obj->imp->hasProperty(exec, i->number())) {
                exec->clearException();
                return false;
            }
        }

        JSLockHolder lock(exec);
        if (i->isString())
            obj->imp->methodTable()->deleteProperty(obj->imp, exec, identifierFromNPIdentifier(exec, i->string()));
        else
            obj->imp->methodTable()->deletePropertyByIndex(obj->imp, exec, i->number());

        exec->clearException();
        return true;
    }
    return false;
}

bool _NPN_HasProperty(NPP, NPObject* o, NPIdentifier propertyName)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 

        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;

        ExecState* exec = rootObject->globalObject()->globalExec();
        IdentifierRep* i = static_cast<IdentifierRep*>(propertyName);
        JSLockHolder lock(exec);
        if (i->isString()) {
            bool result = obj->imp->hasProperty(exec, identifierFromNPIdentifier(exec, i->string()));
            exec->clearException();
            return result;
        }

        bool result = obj->imp->hasProperty(exec, i->number());
        exec->clearException();
        return result;
    }

    if (o->_class->hasProperty)
        return o->_class->hasProperty(o, propertyName);

    return false;
}

bool _NPN_HasMethod(NPP, NPObject* o, NPIdentifier methodName)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 

        IdentifierRep* i = static_cast<IdentifierRep*>(methodName);
        if (!i->isString())
            return false;

        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;

        ExecState* exec = rootObject->globalObject()->globalExec();
        JSLockHolder lock(exec);
        JSValue func = obj->imp->get(exec, identifierFromNPIdentifier(exec, i->string()));
        exec->clearException();
        return !func.isUndefined();
    }
    
    if (o->_class->hasMethod)
        return o->_class->hasMethod(o, methodName);
    
    return false;
}

void _NPN_SetException(NPObject*, const NPUTF8* message)
{
    // Ignoring the NPObject param is consistent with the Mozilla implementation.
    String exception(message);
    CInstance::setGlobalException(exception);
}

bool _NPN_Enumerate(NPP, NPObject* o, NPIdentifier** identifier, uint32_t* count)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o); 
        
        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;
        
        ExecState* exec = rootObject->globalObject()->globalExec();
        JSLockHolder lock(exec);
        PropertyNameArray propertyNames(exec);

        obj->imp->methodTable()->getPropertyNames(obj->imp, exec, propertyNames, ExcludeDontEnumProperties);
        unsigned size = static_cast<unsigned>(propertyNames.size());
        // FIXME: This should really call NPN_MemAlloc but that's in WebKit
        NPIdentifier* identifiers = static_cast<NPIdentifier*>(malloc(sizeof(NPIdentifier) * size));
        
        for (unsigned i = 0; i < size; ++i)
            identifiers[i] = _NPN_GetStringIdentifier(propertyNames[i].string().utf8().data());

        *identifier = identifiers;
        *count = size;

        exec->clearException();
        return true;
    }
    
    if (NP_CLASS_STRUCT_VERSION_HAS_ENUM(o->_class) && o->_class->enumerate)
        return o->_class->enumerate(o, identifier, count);
    
    return false;
}

bool _NPN_Construct(NPP, NPObject* o, const NPVariant* args, uint32_t argCount, NPVariant* result)
{
    if (o->_class == NPScriptObjectClass) {
        JavaScriptObject* obj = reinterpret_cast<JavaScriptObject*>(o);
        
        VOID_TO_NPVARIANT(*result);
        
        // Lookup the constructor object.
        RootObject* rootObject = obj->rootObject;
        if (!rootObject || !rootObject->isValid())
            return false;
        
        ExecState* exec = rootObject->globalObject()->globalExec();
        JSLockHolder lock(exec);
        
        // Call the constructor object.
        JSValue constructor = obj->imp;
        ConstructData constructData;
        ConstructType constructType = getConstructData(constructor, constructData);
        if (constructType == ConstructTypeNone)
            return false;
        
        MarkedArgumentBuffer argList;
        getListFromVariantArgs(exec, args, argCount, rootObject, argList);
        JSValue resultV = JSC::construct(exec, constructor, constructType, constructData, argList);
        
        // Convert and return the result.
        convertValueToNPVariant(exec, resultV, result);
        exec->clearException();
        return true;
    }
    
    if (NP_CLASS_STRUCT_VERSION_HAS_CTOR(o->_class) && o->_class->construct)
        return o->_class->construct(o, args, argCount, result);
    
    return false;
}

#endif // ENABLE(NETSCAPE_PLUGIN_API)
