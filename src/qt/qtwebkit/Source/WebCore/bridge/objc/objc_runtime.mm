/*
 * Copyright (C) 2004, 2008 Apple Inc. All rights reserved.
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
#include "objc_runtime.h"

#include "JSDOMBinding.h"
#include "ObjCRuntimeObject.h"
#include "WebScriptObject.h"
#include "objc_instance.h"
#include "runtime_array.h"
#include "runtime_object.h"
#include <runtime/Error.h>
#include <runtime/JSGlobalObject.h>
#include <runtime/JSLock.h>
#include <wtf/RetainPtr.h>

using namespace WebCore;

namespace JSC {
namespace Bindings {

ClassStructPtr webScriptObjectClass()
{
    static ClassStructPtr<WebScriptObject> webScriptObjectClass = NSClassFromString(@"WebScriptObject");
    return webScriptObjectClass;
}

ClassStructPtr webUndefinedClass()
{
    static ClassStructPtr<WebUndefined> webUndefinedClass = NSClassFromString(@"WebUndefined");
    return webUndefinedClass;
}

// ---------------------- ObjcMethod ----------------------

ObjcMethod::ObjcMethod(ClassStructPtr aClass, SEL selector)
    : _objcClass(aClass)
    , _selector(selector)
{
}

int ObjcMethod::numParameters() const
{
    return [getMethodSignature() numberOfArguments] - 2;
}

NSMethodSignature* ObjcMethod::getMethodSignature() const
{
    return [_objcClass instanceMethodSignatureForSelector:_selector];
}

// ---------------------- ObjcField ----------------------

ObjcField::ObjcField(Ivar ivar) 
    : _ivar(ivar)
    , _name(adoptCF(CFStringCreateWithCString(0, ivar_getName(_ivar), kCFStringEncodingASCII)))
{
}

ObjcField::ObjcField(CFStringRef name)
    : _ivar(0)
    , _name(name)
{
}

JSValue ObjcField::valueFromInstance(ExecState* exec, const Instance* instance) const
{
    JSValue result = jsUndefined();
    
    id targetObject = (static_cast<const ObjcInstance*>(instance))->getObject();

    JSLock::DropAllLocks dropAllLocks(exec); // Can't put this inside the @try scope because it unwinds incorrectly.

    @try {
        if (id objcValue = [targetObject valueForKey:(NSString *)_name.get()])
            result = convertObjcValueToValue(exec, &objcValue, ObjcObjectType, instance->rootObject());
        {
            JSLockHolder lock(exec);
            ObjcInstance::moveGlobalExceptionToExecState(exec);
        }
    } @catch(NSException* localException) {
        JSLockHolder lock(exec);
        throwError(exec, [localException reason]);
    }

    // Work around problem in some versions of GCC where result gets marked volatile and
    // it can't handle copying from a volatile to non-volatile.
    return const_cast<JSValue&>(result);
}

static id convertValueToObjcObject(ExecState* exec, JSValue value)
{
    RefPtr<RootObject> rootObject = findRootObject(exec->dynamicGlobalObject());
    if (!rootObject)
        return nil;
    return [webScriptObjectClass() _convertValueToObjcValue:value originRootObject:rootObject.get() rootObject:rootObject.get()];
}

void ObjcField::setValueToInstance(ExecState* exec, const Instance* instance, JSValue aValue) const
{
    id targetObject = (static_cast<const ObjcInstance*>(instance))->getObject();
    id value = convertValueToObjcObject(exec, aValue);

    JSLock::DropAllLocks dropAllLocks(exec); // Can't put this inside the @try scope because it unwinds incorrectly.

    @try {
        [targetObject setValue:value forKey:(NSString *)_name.get()];
        {
            JSLockHolder lock(exec);
            ObjcInstance::moveGlobalExceptionToExecState(exec);
        }
    } @catch(NSException* localException) {
        JSLockHolder lock(exec);
        throwError(exec, [localException reason]);
    }
}

// ---------------------- ObjcArray ----------------------

ObjcArray::ObjcArray(ObjectStructPtr a, PassRefPtr<RootObject> rootObject)
    : Array(rootObject)
    , _array(a)
{
}

void ObjcArray::setValueAt(ExecState* exec, unsigned int index, JSValue aValue) const
{
    if (![_array.get() respondsToSelector:@selector(insertObject:atIndex:)]) {
        throwError(exec, createTypeError(exec, "Array is not mutable."));
        return;
    }

    if (index > [_array.get() count]) {
        throwError(exec, createRangeError(exec, "Index exceeds array size."));
        return;
    }
    
    // Always try to convert the value to an ObjC object, so it can be placed in the
    // array.
    ObjcValue oValue = convertValueToObjcValue (exec, aValue, ObjcObjectType);

    @try {
        [_array.get() insertObject:oValue.objectValue atIndex:index];
    } @catch(NSException* localException) {
        throwError(exec, createError(exec, "Objective-C exception."));
    }
}

JSValue ObjcArray::valueAt(ExecState* exec, unsigned int index) const
{
    if (index > [_array.get() count])
        return throwError(exec, createRangeError(exec, "Index exceeds array size."));
    @try {
        id obj = [_array.get() objectAtIndex:index];
        if (obj)
            return convertObjcValueToValue (exec, &obj, ObjcObjectType, m_rootObject.get());
    } @catch(NSException* localException) {
        return throwError(exec, createError(exec, "Objective-C exception."));
    }
    return jsUndefined();
}

unsigned int ObjcArray::getLength() const
{
    return [_array.get() count];
}

const ClassInfo ObjcFallbackObjectImp::s_info = { "ObjcFallbackObject", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(ObjcFallbackObjectImp) };

ObjcFallbackObjectImp::ObjcFallbackObjectImp(JSGlobalObject* globalObject, Structure* structure, ObjcInstance* i, const String& propertyName)
    : JSDestructibleObject(globalObject->vm(), structure)
    , _instance(i)
    , m_item(propertyName)
{
}

void ObjcFallbackObjectImp::destroy(JSCell* cell)
{
    static_cast<ObjcFallbackObjectImp*>(cell)->ObjcFallbackObjectImp::~ObjcFallbackObjectImp();
}

void ObjcFallbackObjectImp::finishCreation(JSGlobalObject* globalObject)
{
    Base::finishCreation(globalObject->vm());
    ASSERT(inherits(&s_info));
}

bool ObjcFallbackObjectImp::getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot& slot)
{
    // keep the prototype from getting called instead of just returning false
    slot.setUndefined();
    return true;
}

bool ObjcFallbackObjectImp::getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor& descriptor)
{
    // keep the prototype from getting called instead of just returning false
    descriptor.setUndefined();
    return true;
}

void ObjcFallbackObjectImp::put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&)
{
}

static EncodedJSValue JSC_HOST_CALL callObjCFallbackObject(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&ObjCRuntimeObject::s_info))
        return throwVMTypeError(exec);

    JSValue result = jsUndefined();

    ObjCRuntimeObject* runtimeObject = static_cast<ObjCRuntimeObject*>(asObject(thisValue));
    ObjcInstance* objcInstance = runtimeObject->getInternalObjCInstance();

    if (!objcInstance)
        return JSValue::encode(RuntimeObject::throwInvalidAccessError(exec));
    
    objcInstance->begin();

    id targetObject = objcInstance->getObject();
    
    if ([targetObject respondsToSelector:@selector(invokeUndefinedMethodFromWebScript:withArguments:)]){
        ObjcClass* objcClass = static_cast<ObjcClass*>(objcInstance->getClass());
        OwnPtr<ObjcMethod> fallbackMethod(adoptPtr(new ObjcMethod(objcClass->isa(), @selector(invokeUndefinedMethodFromWebScript:withArguments:))));
        const String& nameIdentifier = static_cast<ObjcFallbackObjectImp*>(exec->callee())->propertyName();
        fallbackMethod->setJavaScriptName(nameIdentifier.createCFString().get());
        result = objcInstance->invokeObjcMethod(exec, fallbackMethod.get());
    }
            
    objcInstance->end();

    return JSValue::encode(result);
}

CallType ObjcFallbackObjectImp::getCallData(JSCell* cell, CallData& callData)
{
    ObjcFallbackObjectImp* thisObject = jsCast<ObjcFallbackObjectImp*>(cell);
    id targetObject = thisObject->_instance->getObject();
    if (![targetObject respondsToSelector:@selector(invokeUndefinedMethodFromWebScript:withArguments:)])
        return CallTypeNone;
    callData.native.function = callObjCFallbackObject;
    return CallTypeHost;
}

bool ObjcFallbackObjectImp::deleteProperty(JSCell*, ExecState*, PropertyName)
{
    return false;
}

JSValue ObjcFallbackObjectImp::defaultValue(const JSObject* object, ExecState* exec, PreferredPrimitiveType)
{
    const ObjcFallbackObjectImp* thisObject = jsCast<const ObjcFallbackObjectImp*>(object);
    return thisObject->_instance->getValueOfUndefinedField(exec, Identifier(exec, thisObject->m_item));
}

bool ObjcFallbackObjectImp::toBoolean(ExecState *) const
{
    id targetObject = _instance->getObject();
    
    if ([targetObject respondsToSelector:@selector(invokeUndefinedMethodFromWebScript:withArguments:)])
        return true;
    
    return false;
}

}
}
