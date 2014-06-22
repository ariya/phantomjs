/*
 * Copyright (C) 2004, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#import "WebScriptObjectPrivate.h"

#import "BindingSecurity.h"
#import "BridgeJSC.h"
#import "Console.h"
#import "DOMInternal.h"
#import "DOMWindow.h"
#import "Frame.h"
#import "JSDOMWindow.h"
#import "JSDOMWindowCustom.h"
#import "JSHTMLElement.h"
#import "JSMainThreadExecState.h"
#import "JSPluginElementFunctions.h"
#import "ObjCRuntimeObject.h"
#import "WebCoreObjCExtras.h"
#import "objc_instance.h"
#import "runtime_object.h"
#import "runtime_root.h"
#import <JavaScriptCore/APICast.h>
#import <JavaScriptCore/JSContextInternal.h>
#import <JavaScriptCore/JSValueInternal.h>
#import <interpreter/CallFrame.h>
#import <runtime/InitializeThreading.h>
#import <runtime/JSGlobalObject.h>
#import <runtime/JSLock.h>
#import <runtime/Completion.h>
#import <runtime/Completion.h>
#import <wtf/TCSpinLock.h>
#import <wtf/Threading.h>
#include <wtf/text/WTFString.h>

using namespace JSC::Bindings;
using namespace WebCore;

using JSC::CallData;
using JSC::CallType;
using JSC::CallTypeNone;
using JSC::ExecState;
using JSC::Identifier;
using JSC::JSLockHolder;
using JSC::JSObject;
using JSC::MarkedArgumentBuffer;
using JSC::PutPropertySlot;
using JSC::jsCast;
using JSC::jsUndefined;
using JSC::makeSource;

namespace WebCore {

static NSMapTable* JSWrapperCache;
static SpinLock spinLock = SPINLOCK_INITIALIZER;

NSObject* getJSWrapper(JSObject* impl)
{
    ASSERT(isMainThread());
    SpinLockHolder holder(&spinLock);

    if (!JSWrapperCache)
        return nil;
    NSObject* wrapper = static_cast<NSObject*>(NSMapGet(JSWrapperCache, impl));
    return wrapper ? [[wrapper retain] autorelease] : nil;
}

void addJSWrapper(NSObject* wrapper, JSObject* impl)
{
    ASSERT(isMainThread());
    SpinLockHolder holder(&spinLock);

    if (!JSWrapperCache)
        JSWrapperCache = createWrapperCache();
    NSMapInsert(JSWrapperCache, impl, wrapper);
}

void removeJSWrapper(JSObject* impl)
{
    SpinLockHolder holder(&spinLock);

    if (!JSWrapperCache)
        return;
    NSMapRemove(JSWrapperCache, impl);
}

static void removeJSWrapperIfRetainCountOne(NSObject* wrapper, JSObject* impl)
{
    SpinLockHolder holder(&spinLock);

    if (!JSWrapperCache)
        return;
    if ([wrapper retainCount] == 1)
        NSMapRemove(JSWrapperCache, impl);
}

id createJSWrapper(JSC::JSObject* object, PassRefPtr<JSC::Bindings::RootObject> origin, PassRefPtr<JSC::Bindings::RootObject> root)
{
    if (id wrapper = getJSWrapper(object))
        return wrapper;
    return [[[WebScriptObject alloc] _initWithJSObject:object originRootObject:origin rootObject:root] autorelease];
}

static void addExceptionToConsole(ExecState* exec)
{
    JSDOMWindow* window = asJSDOMWindow(exec->dynamicGlobalObject());
    if (!window || !exec->hadException())
        return;
    reportCurrentException(exec);
}

} // namespace WebCore

@implementation WebScriptObjectPrivate

@end

@implementation WebScriptObject

+ (void)initialize
{
#if !USE(WEB_THREAD)
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
#endif // !USE(WEB_THREAD)
    WebCoreObjCFinalizeOnMainThread(self);
}

+ (id)scriptObjectForJSObject:(JSObjectRef)jsObject originRootObject:(RootObject*)originRootObject rootObject:(RootObject*)rootObject
{
    if (id domWrapper = createDOMWrapper(toJS(jsObject), originRootObject, rootObject))
        return domWrapper;
    
    return WebCore::createJSWrapper(toJS(jsObject), originRootObject, rootObject);
}

- (void)_setImp:(JSObject*)imp originRootObject:(PassRefPtr<RootObject>)originRootObject rootObject:(PassRefPtr<RootObject>)rootObject
{
    // This function should only be called once, as a (possibly lazy) initializer.
    ASSERT(!_private->imp);
    ASSERT(!_private->rootObject);
    ASSERT(!_private->originRootObject);
    ASSERT(imp);

    _private->imp = imp;
    _private->rootObject = rootObject.leakRef();
    _private->originRootObject = originRootObject.leakRef();

    WebCore::addJSWrapper(self, imp);

    if (_private->rootObject)
        _private->rootObject->gcProtect(imp);
}

- (void)_setOriginRootObject:(PassRefPtr<RootObject>)originRootObject andRootObject:(PassRefPtr<RootObject>)rootObject
{
    ASSERT(_private->imp);

    if (rootObject)
        rootObject->gcProtect(_private->imp);

    if (_private->rootObject && _private->rootObject->isValid())
        _private->rootObject->gcUnprotect(_private->imp);

    if (_private->rootObject)
        _private->rootObject->deref();

    if (_private->originRootObject)
        _private->originRootObject->deref();

    _private->rootObject = rootObject.leakRef();
    _private->originRootObject = originRootObject.leakRef();
}

- (id)_initWithJSObject:(JSC::JSObject*)imp originRootObject:(PassRefPtr<JSC::Bindings::RootObject>)originRootObject rootObject:(PassRefPtr<JSC::Bindings::RootObject>)rootObject
{
    ASSERT(imp);

    self = [super init];
    _private = [[WebScriptObjectPrivate alloc] init];
    [self _setImp:imp originRootObject:originRootObject rootObject:rootObject];
    
    return self;
}

- (JSObject*)_imp
{
    // Associate the WebScriptObject with the JS wrapper for the ObjC DOM wrapper.
    // This is done on lazily, on demand.
    if (!_private->imp && _private->isCreatedByDOMWrapper)
        [self _initializeScriptDOMNodeImp];
    return [self _rootObject] ? _private->imp : 0;
}

- (BOOL)_hasImp
{
    return _private->imp != nil;
}

// Node that DOMNode overrides this method. So you should almost always
// use this method call instead of _private->rootObject directly.
- (RootObject*)_rootObject
{
    return _private->rootObject && _private->rootObject->isValid() ? _private->rootObject : 0;
}

- (RootObject *)_originRootObject
{
    return _private->originRootObject && _private->originRootObject->isValid() ? _private->originRootObject : 0;
}

- (BOOL)_isSafeScript
{
    RootObject *root = [self _rootObject];
    if (!root)
        return false;

    if (!_private->originRootObject)
        return true;

    if (!_private->originRootObject->isValid())
        return false;

    // It's not actually correct to call shouldAllowAccessToFrame in this way because
    // JSDOMWindowBase* isn't the right object to represent the currently executing
    // JavaScript. Instead, we should use ExecState, like we do elsewhere.
    JSDOMWindowBase* target = jsCast<JSDOMWindowBase*>(root->globalObject());
    return BindingSecurity::shouldAllowAccessToDOMWindow(_private->originRootObject->globalObject()->globalExec(), target->impl());
}

- (JSGlobalContextRef)_globalContextRef
{
    if (![self _isSafeScript])
        return nil;
    return toGlobalRef([self _rootObject]->globalObject()->globalExec());
}

- (oneway void)release
{
    // If we're releasing the last reference to this object, remove if from the map.
    if (_private->imp)
        WebCore::removeJSWrapperIfRetainCountOne(self, _private->imp);

    [super release];
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebScriptObject class], self))
        return;

    if (_private->rootObject && _private->rootObject->isValid())
        _private->rootObject->gcUnprotect(_private->imp);

    if (_private->rootObject)
        _private->rootObject->deref();

    if (_private->originRootObject)
        _private->originRootObject->deref();

    [_private release];

    [super dealloc];
}

- (void)finalize
{
    if (_private->rootObject && _private->rootObject->isValid())
        _private->rootObject->gcUnprotect(_private->imp);

    if (_private->rootObject)
        _private->rootObject->deref();

    if (_private->originRootObject)
        _private->originRootObject->deref();

    [super finalize];
}

+ (BOOL)throwException:(NSString *)exceptionMessage
{
    ObjcInstance::setGlobalException(exceptionMessage);
    return YES;
}

static void getListFromNSArray(ExecState *exec, NSArray *array, RootObject* rootObject, MarkedArgumentBuffer& aList)
{
    int i, numObjects = array ? [array count] : 0;
    
    for (i = 0; i < numObjects; i++) {
        id anObject = [array objectAtIndex:i];
        aList.append(convertObjcValueToValue(exec, &anObject, ObjcObjectType, rootObject));
    }
}

- (id)callWebScriptMethod:(NSString *)name withArguments:(NSArray *)args
{
    if (![self _isSafeScript])
        return nil;

    // Look up the function object.
    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    JSLockHolder lock(exec);
    ASSERT(!exec->hadException());

    JSC::JSValue function = [self _imp]->get(exec, Identifier(exec, String(name)));
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return nil;

    MarkedArgumentBuffer argList;
    getListFromNSArray(exec, args, [self _rootObject], argList);

    if (![self _isSafeScript])
        return nil;

    JSC::JSValue result = JSMainThreadExecState::call(exec, function, callType, callData, [self _imp], argList);

    if (exec->hadException()) {
        addExceptionToConsole(exec);
        result = jsUndefined();
        exec->clearException();
    }

    // Convert and return the result of the function call.
    id resultObj = [WebScriptObject _convertValueToObjcValue:result originRootObject:[self _originRootObject] rootObject:[self _rootObject]];

    return resultObj;
}

- (id)evaluateWebScript:(NSString *)script
{
    if (![self _isSafeScript])
        return nil;
    
    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    ASSERT(!exec->hadException());

    JSLockHolder lock(exec);
    
    JSC::JSValue returnValue = JSMainThreadExecState::evaluate(exec, makeSource(String(script)), JSC::JSValue(), 0);

    id resultObj = [WebScriptObject _convertValueToObjcValue:returnValue originRootObject:[self _originRootObject] rootObject:[self _rootObject]];
    
    return resultObj;
}

- (void)setValue:(id)value forKey:(NSString *)key
{
    if (![self _isSafeScript])
        return;

    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    ASSERT(!exec->hadException());

    JSLockHolder lock(exec);

    PutPropertySlot slot;
    [self _imp]->methodTable()->put([self _imp], exec, Identifier(exec, String(key)), convertObjcValueToValue(exec, &value, ObjcObjectType, [self _rootObject]), slot);

    if (exec->hadException()) {
        addExceptionToConsole(exec);
        exec->clearException();
    }
}

- (id)valueForKey:(NSString *)key
{
    if (![self _isSafeScript])
        return nil;

    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    ASSERT(!exec->hadException());

    id resultObj;
    {
        // Need to scope this lock to ensure that we release the lock before calling
        // [super valueForKey:key] which might throw an exception and bypass the JSLock destructor,
        // leaving the lock permanently held
        JSLockHolder lock(exec);
        
        JSC::JSValue result = [self _imp]->get(exec, Identifier(exec, String(key)));
        
        if (exec->hadException()) {
            addExceptionToConsole(exec);
            result = jsUndefined();
            exec->clearException();
        }

        resultObj = [WebScriptObject _convertValueToObjcValue:result originRootObject:[self _originRootObject] rootObject:[self _rootObject]];
    }
    
    if ([resultObj isKindOfClass:[WebUndefined class]])
        resultObj = [super valueForKey:key];    // defaults to throwing an exception

    return resultObj;
}

- (void)removeWebScriptKey:(NSString *)key
{
    if (![self _isSafeScript])
        return;

    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    ASSERT(!exec->hadException());

    JSLockHolder lock(exec);
    [self _imp]->methodTable()->deleteProperty([self _imp], exec, Identifier(exec, String(key)));

    if (exec->hadException()) {
        addExceptionToConsole(exec);
        exec->clearException();
    }
}

- (BOOL)hasWebScriptKey:(NSString *)key
{
    if (![self _isSafeScript])
        return NO;

    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    ASSERT(!exec->hadException());

    JSLockHolder lock(exec);
    BOOL result = [self _imp]->hasProperty(exec, Identifier(exec, String(key)));

    if (exec->hadException()) {
        addExceptionToConsole(exec);
        exec->clearException();
    }

    return result;
}

- (NSString *)stringRepresentation
{
    if (![self _isSafeScript]) {
        // This is a workaround for a gcc 3.3 internal compiler error.
        return @"Undefined";
    }

    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    JSLockHolder lock(exec);
    
    id result = convertValueToObjcValue(exec, [self _imp], ObjcObjectType).objectValue;

    NSString *description = [result description];

    return description;
}

- (id)webScriptValueAtIndex:(unsigned)index
{
    if (![self _isSafeScript])
        return nil;

    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    ASSERT(!exec->hadException());

    JSLockHolder lock(exec);
    JSC::JSValue result = [self _imp]->get(exec, index);

    if (exec->hadException()) {
        addExceptionToConsole(exec);
        result = jsUndefined();
        exec->clearException();
    }

    id resultObj = [WebScriptObject _convertValueToObjcValue:result originRootObject:[self _originRootObject] rootObject:[self _rootObject]];

    return resultObj;
}

- (void)setWebScriptValueAtIndex:(unsigned)index value:(id)value
{
    if (![self _isSafeScript])
        return;

    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    ASSERT(!exec->hadException());

    JSLockHolder lock(exec);
    [self _imp]->methodTable()->putByIndex([self _imp], exec, index, convertObjcValueToValue(exec, &value, ObjcObjectType, [self _rootObject]), false);

    if (exec->hadException()) {
        addExceptionToConsole(exec);
        exec->clearException();
    }
}

- (void)setException:(NSString *)description
{
    if (![self _rootObject])
        return;
    ObjcInstance::setGlobalException(description, [self _rootObject]->globalObject());
}

- (JSObjectRef)JSObject
{
    if (![self _isSafeScript])
        return 0;
    ExecState* exec = [self _rootObject]->globalObject()->globalExec();
    
    JSLockHolder lock(exec);
    return toRef([self _imp]);
}

+ (id)_convertValueToObjcValue:(JSC::JSValue)value originRootObject:(RootObject*)originRootObject rootObject:(RootObject*)rootObject
{
    if (value.isObject()) {
        JSObject* object = asObject(value);
        JSLockHolder lock(rootObject->globalObject()->vm());

        if (object->inherits(&JSHTMLElement::s_info)) {
            // Plugin elements cache the instance internally.
            HTMLElement* el = jsCast<JSHTMLElement*>(object)->impl();
            ObjcInstance* instance = static_cast<ObjcInstance*>(pluginInstance(el));
            if (instance)
                return instance->getObject();
        } else if (object->inherits(&ObjCRuntimeObject::s_info)) {
            ObjCRuntimeObject* runtimeObject = static_cast<ObjCRuntimeObject*>(object);
            ObjcInstance* instance = runtimeObject->getInternalObjCInstance();
            if (instance)
                return instance->getObject();
            return nil;
        }

        return [WebScriptObject scriptObjectForJSObject:toRef(object) originRootObject:originRootObject rootObject:rootObject];
    }

    if (value.isString()) {
        ExecState* exec = rootObject->globalObject()->globalExec();
        const String& u = asString(value)->value(exec);
        return [NSString stringWithCharacters:u.characters() length:u.length()];
    }

    if (value.isNumber())
        return [NSNumber numberWithDouble:value.asNumber()];

    if (value.isBoolean())
        return [NSNumber numberWithBool:value.asBoolean()];

    if (value.isUndefined())
        return [WebUndefined undefined];

    // jsNull is not returned as NSNull because existing applications do not expect
    // that return value. Return as nil for compatibility. <rdar://problem/4651318> <rdar://problem/4701626>
    // Other types (e.g., UnspecifiedType) also return as nil.
    return nil;
}


#if JSC_OBJC_API_ENABLED
- (JSValue *)JSValue
{
    if (![self _isSafeScript])
        return 0;
    
    return [JSValue valueWithJSValueRef:[self JSObject] 
                    inContext:[JSContext contextWithJSGlobalContextRef:[self _globalContextRef]]];
}
#endif

@end

@interface WebScriptObject (WebKitCocoaBindings)

- (id)objectAtIndex:(unsigned)index;

@end

@implementation WebScriptObject (WebKitCocoaBindings)

#if 0 

// FIXME: We'd like to add this, but we can't do that until this issue is resolved:
// http://bugs.webkit.org/show_bug.cgi?id=13129: presence of 'count' method on
// WebScriptObject breaks Democracy player.

- (unsigned)count
{
    id length = [self valueForKey:@"length"];
    if (![length respondsToSelector:@selector(intValue)])
        return 0;
    return [length intValue];
}

#endif

- (id)objectAtIndex:(unsigned)index
{
    return [self webScriptValueAtIndex:index];
}

@end

@implementation WebUndefined

+ (id)allocWithZone:(NSZone *)unusedZone
{
    UNUSED_PARAM(unusedZone);

    static WebUndefined *sharedUndefined = 0;
    if (!sharedUndefined)
        sharedUndefined = [super allocWithZone:NULL];
    return sharedUndefined;
}

- (NSString *)description
{
    return @"undefined";
}

- (id)initWithCoder:(NSCoder *)unusedCoder
{
    UNUSED_PARAM(unusedCoder);

    return self;
}

- (void)encodeWithCoder:(NSCoder *)unusedCoder
{
    UNUSED_PARAM(unusedCoder);
}

- (id)copyWithZone:(NSZone *)unusedZone
{
    UNUSED_PARAM(unusedZone);

    return self;
}

- (id)retain
{
    return self;
}

- (oneway void)release
{
}

- (NSUInteger)retainCount
{
    return NSUIntegerMax;
}

- (id)autorelease
{
    return self;
}

- (void)dealloc
{
    return;
    [super dealloc]; // make -Wdealloc-check happy
}

+ (WebUndefined *)undefined
{
    return [WebUndefined allocWithZone:NULL];
}

@end
