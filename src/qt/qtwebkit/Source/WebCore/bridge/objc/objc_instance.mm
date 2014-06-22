/*
 * Copyright (C) 2004, 2008, 2009 Apple Inc. All rights reserved.
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

#import "config.h"
#import "objc_instance.h"

#import "runtime_method.h"
#import <runtime/ObjectPrototype.h>
#import "JSDOMBinding.h"
#import "ObjCRuntimeObject.h"
#import "WebScriptObject.h"
#import <objc/objc-auto.h>
#import <runtime/Error.h>
#import <runtime/JSLock.h>
#import "runtime/FunctionPrototype.h"
#import <wtf/Assertions.h>

#ifdef NDEBUG
#define OBJC_LOG(formatAndArgs...) ((void)0)
#else
#define OBJC_LOG(formatAndArgs...) { \
    fprintf (stderr, "%s:%d -- %s:  ", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(stderr, formatAndArgs); \
}
#endif

using namespace JSC::Bindings;
using namespace JSC;

static NSString *s_exception;
static JSGlobalObject* s_exceptionEnvironment; // No need to protect this value, since we just use it for a pointer comparison.
static NSMapTable *s_instanceWrapperCache;

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif    

static NSMapTable *createInstanceWrapperCache()
{
    // NSMapTable with zeroing weak pointers is the recommended way to build caches like this under garbage collection.
    NSPointerFunctionsOptions keyOptions = NSPointerFunctionsZeroingWeakMemory | NSPointerFunctionsOpaquePersonality;
    NSPointerFunctionsOptions valueOptions = NSPointerFunctionsOpaqueMemory | NSPointerFunctionsOpaquePersonality;
    return [[NSMapTable alloc] initWithKeyOptions:keyOptions valueOptions:valueOptions capacity:0];
}

#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

RuntimeObject* ObjcInstance::newRuntimeObject(ExecState* exec)
{
    return ObjCRuntimeObject::create(exec, exec->lexicalGlobalObject(), this);
}

void ObjcInstance::setGlobalException(NSString* exception, JSGlobalObject* exceptionEnvironment)
{
    NSString *oldException = s_exception;
    s_exception = [exception copy];
    [oldException release];

    s_exceptionEnvironment = exceptionEnvironment;
}

void ObjcInstance::moveGlobalExceptionToExecState(ExecState* exec)
{
    if (!s_exception) {
        ASSERT(!s_exceptionEnvironment);
        return;
    }

    if (!s_exceptionEnvironment || s_exceptionEnvironment == exec->dynamicGlobalObject()) {
        JSLockHolder lock(exec);
        throwError(exec, s_exception);
    }

    [s_exception release];
    s_exception = nil;
    s_exceptionEnvironment = 0;
}

ObjcInstance::ObjcInstance(id instance, PassRefPtr<RootObject> rootObject) 
    : Instance(rootObject)
    , _instance(instance)
    , _class(0)
    , _pool(0)
    , _beginCount(0)
{
}

PassRefPtr<ObjcInstance> ObjcInstance::create(id instance, PassRefPtr<RootObject> rootObject)
{
    if (!s_instanceWrapperCache)
        s_instanceWrapperCache = createInstanceWrapperCache();
    if (void* existingWrapper = NSMapGet(s_instanceWrapperCache, instance))
        return static_cast<ObjcInstance*>(existingWrapper);
    RefPtr<ObjcInstance> wrapper = adoptRef(new ObjcInstance(instance, rootObject));
    NSMapInsert(s_instanceWrapperCache, instance, wrapper.get());
    return wrapper.release();
}

ObjcInstance::~ObjcInstance() 
{
    // Both -finalizeForWebScript and -dealloc/-finalize of _instance may require autorelease pools.
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    ASSERT(s_instanceWrapperCache);
    ASSERT(_instance);
    NSMapRemove(s_instanceWrapperCache, _instance.get());

    if ([_instance.get() respondsToSelector:@selector(finalizeForWebScript)])
        [_instance.get() performSelector:@selector(finalizeForWebScript)];
    _instance = 0;

    [pool drain];
}

static NSAutoreleasePool* allocateAutoReleasePool()
{
    // If GC is enabled an autorelease pool is unnecessary, and the
    // pool cannot be protected from GC so may be collected leading
    // to a crash when we try to drain the release pool.
    if (objc_collectingEnabled())
        return nil;

    return [[NSAutoreleasePool alloc] init];
}

void ObjcInstance::virtualBegin()
{
    if (!_pool)
        _pool = allocateAutoReleasePool();
    _beginCount++;
}

void ObjcInstance::virtualEnd()
{
    _beginCount--;
    ASSERT(_beginCount >= 0);
    if (!_beginCount) {
        [_pool drain];
        _pool = 0;
    }
}

Bindings::Class* ObjcInstance::getClass() const 
{
    if (!_instance)
        return 0;
    if (!_class)
        _class = ObjcClass::classForIsA(object_getClass(_instance.get()));
    return static_cast<Bindings::Class*>(_class);
}

bool ObjcInstance::supportsInvokeDefaultMethod() const
{
    return [_instance.get() respondsToSelector:@selector(invokeDefaultMethodWithArguments:)];
}

class ObjCRuntimeMethod : public RuntimeMethod {
public:
    static ObjCRuntimeMethod* create(ExecState* exec, JSGlobalObject* globalObject, const String& name, Bindings::Method* method)
    {
        // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
        // We need to pass in the right global object for "i".
        Structure* domStructure = WebCore::deprecatedGetDOMStructure<ObjCRuntimeMethod>(exec);
        ObjCRuntimeMethod* runtimeMethod = new (NotNull, allocateCell<ObjCRuntimeMethod>(*exec->heap())) ObjCRuntimeMethod(globalObject, domStructure, method);
        runtimeMethod->finishCreation(exec->vm(), name);
        return runtimeMethod;
    }

    static Structure* createStructure(VM& vm, JSC::JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }

    static const ClassInfo s_info;

private:
    typedef RuntimeMethod Base;

    ObjCRuntimeMethod(JSGlobalObject* globalObject, Structure* structure, Bindings::Method* method)
        : RuntimeMethod(globalObject, structure, method)
    {
    }

    void finishCreation(VM& vm, const String& name)
    {
        Base::finishCreation(vm, name);
        ASSERT(inherits(&s_info));
    }
};

const ClassInfo ObjCRuntimeMethod::s_info = { "ObjCRuntimeMethod", &RuntimeMethod::s_info, 0, 0, CREATE_METHOD_TABLE(ObjCRuntimeMethod) };

JSValue ObjcInstance::getMethod(ExecState* exec, PropertyName propertyName)
{
    Method* method = getClass()->methodNamed(propertyName, this);
    return ObjCRuntimeMethod::create(exec, exec->lexicalGlobalObject(), propertyName.publicName(), method);
}

JSValue ObjcInstance::invokeMethod(ExecState* exec, RuntimeMethod* runtimeMethod)
{
    if (!asObject(runtimeMethod)->inherits(&ObjCRuntimeMethod::s_info))
        return throwError(exec, createTypeError(exec, "Attempt to invoke non-plug-in method on plug-in object."));

    ObjcMethod *method = static_cast<ObjcMethod*>(runtimeMethod->method());
    ASSERT(method);

    return invokeObjcMethod(exec, method);
}

JSValue ObjcInstance::invokeObjcMethod(ExecState* exec, ObjcMethod* method)
{
    JSValue result = jsUndefined();
    
    JSLock::DropAllLocks dropAllLocks(exec); // Can't put this inside the @try scope because it unwinds incorrectly.

    setGlobalException(nil);
    
@try {
    NSMethodSignature* signature = method->getMethodSignature();
    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setSelector:method->selector()];
    [invocation setTarget:_instance.get()];

    if (method->isFallbackMethod()) {
        if (objcValueTypeForType([signature methodReturnType]) != ObjcObjectType) {
            NSLog(@"Incorrect signature for invokeUndefinedMethodFromWebScript:withArguments: -- return type must be object.");
            return result;
        }

        // Invoke invokeUndefinedMethodFromWebScript:withArguments:, pass JavaScript function
        // name as first (actually at 2) argument and array of args as second.
        NSString* jsName = (NSString* )method->javaScriptName();
        [invocation setArgument:&jsName atIndex:2];

        NSMutableArray* objcArgs = [NSMutableArray array];
        int count = exec->argumentCount();
        for (int i = 0; i < count; i++) {
            ObjcValue value = convertValueToObjcValue(exec, exec->argument(i), ObjcObjectType);
            [objcArgs addObject:value.objectValue];
        }
        [invocation setArgument:&objcArgs atIndex:3];
    } else {
        unsigned count = [signature numberOfArguments];
        for (unsigned i = 2; i < count ; i++) {
            const char* type = [signature getArgumentTypeAtIndex:i];
            ObjcValueType objcValueType = objcValueTypeForType(type);

            // Must have a valid argument type.  This method signature should have
            // been filtered already to ensure that it has acceptable argument
            // types.
            ASSERT(objcValueType != ObjcInvalidType && objcValueType != ObjcVoidType);

            ObjcValue value = convertValueToObjcValue(exec, exec->argument(i-2), objcValueType);

            switch (objcValueType) {
                case ObjcObjectType:
                    [invocation setArgument:&value.objectValue atIndex:i];
                    break;
                case ObjcCharType:
                case ObjcUnsignedCharType:
                    [invocation setArgument:&value.charValue atIndex:i];
                    break;
                case ObjcShortType:
                case ObjcUnsignedShortType:
                    [invocation setArgument:&value.shortValue atIndex:i];
                    break;
                case ObjcBoolType:
                    [invocation setArgument:&value.booleanValue atIndex:i];
                    break;
                case ObjcIntType:
                case ObjcUnsignedIntType:
                    [invocation setArgument:&value.intValue atIndex:i];
                    break;
                case ObjcLongType:
                case ObjcUnsignedLongType:
                    [invocation setArgument:&value.longValue atIndex:i];
                    break;
                case ObjcLongLongType:
                case ObjcUnsignedLongLongType:
                    [invocation setArgument:&value.longLongValue atIndex:i];
                    break;
                case ObjcFloatType:
                    [invocation setArgument:&value.floatValue atIndex:i];
                    break;
                case ObjcDoubleType:
                    [invocation setArgument:&value.doubleValue atIndex:i];
                    break;
                default:
                    // Should never get here.  Argument types are filtered (and
                    // the assert above should have fired in the impossible case
                    // of an invalid type anyway).
                    fprintf(stderr, "%s: invalid type (%d)\n", __PRETTY_FUNCTION__, (int)objcValueType);
                    ASSERT_NOT_REACHED();
            }
        }
    }

    [invocation invoke];

    // Get the return value type.
    const char* type = [signature methodReturnType];
    ObjcValueType objcValueType = objcValueTypeForType(type);

    // Must have a valid return type.  This method signature should have
    // been filtered already to ensure that it have an acceptable return
    // type.
    ASSERT(objcValueType != ObjcInvalidType);

    // Get the return value and convert it to a JavaScript value. Length
    // of return value will never exceed the size of largest scalar
    // or a pointer.
    char buffer[1024];
    ASSERT([signature methodReturnLength] < 1024);

    if (*type != 'v') {
        [invocation getReturnValue:buffer];
        result = convertObjcValueToValue(exec, buffer, objcValueType, m_rootObject.get());
    }
} @catch(NSException* localException) {
}
    moveGlobalExceptionToExecState(exec);

    // Work around problem in some versions of GCC where result gets marked volatile and
    // it can't handle copying from a volatile to non-volatile.
    return const_cast<JSValue&>(result);
}

JSValue ObjcInstance::invokeDefaultMethod(ExecState* exec)
{
    JSValue result = jsUndefined();

    JSLock::DropAllLocks dropAllLocks(exec); // Can't put this inside the @try scope because it unwinds incorrectly.
    setGlobalException(nil);
    
@try {
    if (![_instance.get() respondsToSelector:@selector(invokeDefaultMethodWithArguments:)])
        return result;

    NSMethodSignature* signature = [_instance.get() methodSignatureForSelector:@selector(invokeDefaultMethodWithArguments:)];
    NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setSelector:@selector(invokeDefaultMethodWithArguments:)];
    [invocation setTarget:_instance.get()];

    if (objcValueTypeForType([signature methodReturnType]) != ObjcObjectType) {
        NSLog(@"Incorrect signature for invokeDefaultMethodWithArguments: -- return type must be object.");
        return result;
    }

    NSMutableArray* objcArgs = [NSMutableArray array];
    unsigned count = exec->argumentCount();
    for (unsigned i = 0; i < count; i++) {
        ObjcValue value = convertValueToObjcValue(exec, exec->argument(i), ObjcObjectType);
        [objcArgs addObject:value.objectValue];
    }
    [invocation setArgument:&objcArgs atIndex:2];

    [invocation invoke];

    // Get the return value type, should always be "@" because of
    // check above.
    const char* type = [signature methodReturnType];
    ObjcValueType objcValueType = objcValueTypeForType(type);

    // Get the return value and convert it to a JavaScript value. Length
    // of return value will never exceed the size of a pointer, so we're
    // OK with 32 here.
    char buffer[32];
    [invocation getReturnValue:buffer];
    result = convertObjcValueToValue(exec, buffer, objcValueType, m_rootObject.get());
} @catch(NSException* localException) {
}
    moveGlobalExceptionToExecState(exec);

    // Work around problem in some versions of GCC where result gets marked volatile and
    // it can't handle copying from a volatile to non-volatile.
    return const_cast<JSValue&>(result);
}

bool ObjcInstance::setValueOfUndefinedField(ExecState* exec, PropertyName propertyName, JSValue aValue)
{
    String name(propertyName.publicName());
    if (name.isNull())
        return false;

    id targetObject = getObject();
    if (![targetObject respondsToSelector:@selector(setValue:forUndefinedKey:)])
        return false;

    JSLock::DropAllLocks dropAllLocks(exec); // Can't put this inside the @try scope because it unwinds incorrectly.

    // This check is not really necessary because NSObject implements
    // setValue:forUndefinedKey:, and unfortunately the default implementation
    // throws an exception.
    if ([targetObject respondsToSelector:@selector(setValue:forUndefinedKey:)]){
        setGlobalException(nil);
    
        ObjcValue objcValue = convertValueToObjcValue(exec, aValue, ObjcObjectType);

        @try {
            [targetObject setValue:objcValue.objectValue forUndefinedKey:[NSString stringWithCString:name.ascii().data() encoding:NSASCIIStringEncoding]];
        } @catch(NSException* localException) {
            // Do nothing.  Class did not override valueForUndefinedKey:.
        }

        moveGlobalExceptionToExecState(exec);
    }
    
    return true;
}

JSValue ObjcInstance::getValueOfUndefinedField(ExecState* exec, PropertyName propertyName) const
{
    String name(propertyName.publicName());
    if (name.isNull())
        return jsUndefined();

    JSValue result = jsUndefined();
    
    id targetObject = getObject();

    JSLock::DropAllLocks dropAllLocks(exec); // Can't put this inside the @try scope because it unwinds incorrectly.

    // This check is not really necessary because NSObject implements
    // valueForUndefinedKey:, and unfortunately the default implementation
    // throws an exception.
    if ([targetObject respondsToSelector:@selector(valueForUndefinedKey:)]){
        setGlobalException(nil);
    
        @try {
            id objcValue = [targetObject valueForUndefinedKey:[NSString stringWithCString:name.ascii().data() encoding:NSASCIIStringEncoding]];
            result = convertObjcValueToValue(exec, &objcValue, ObjcObjectType, m_rootObject.get());
        } @catch(NSException* localException) {
            // Do nothing.  Class did not override valueForUndefinedKey:.
        }

        moveGlobalExceptionToExecState(exec);
    }

    // Work around problem in some versions of GCC where result gets marked volatile and
    // it can't handle copying from a volatile to non-volatile.
    return const_cast<JSValue&>(result);
}

JSValue ObjcInstance::defaultValue(ExecState* exec, PreferredPrimitiveType hint) const
{
    if (hint == PreferString)
        return stringValue(exec);
    if (hint == PreferNumber)
        return numberValue(exec);
    if ([_instance.get() isKindOfClass:[NSString class]])
        return stringValue(exec);
    if ([_instance.get() isKindOfClass:[NSNumber class]])
        return numberValue(exec);
    return valueOf(exec);
}

JSValue ObjcInstance::stringValue(ExecState* exec) const
{
    return convertNSStringToString(exec, [getObject() description]);
}

JSValue ObjcInstance::numberValue(ExecState*) const
{
    // FIXME:  Implement something sensible
    return jsNumber(0);
}

JSValue ObjcInstance::booleanValue() const
{
    // FIXME:  Implement something sensible
    return jsBoolean(false);
}

JSValue ObjcInstance::valueOf(ExecState* exec) const 
{
    return stringValue(exec);
}
