/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#import "JavaScriptCore.h"

#if JSC_OBJC_API_ENABLED

#import "APICast.h"
#import "APIShims.h"
#import "JSVirtualMachine.h"
#import "JSVirtualMachineInternal.h"
#import "JSWrapperMap.h"

static NSMapTable *globalWrapperCache = 0;

static Mutex& wrapperCacheLock()
{
    DEFINE_STATIC_LOCAL(Mutex, mutex, ());
    return mutex;
}

static void initWrapperCache()
{
    ASSERT(!globalWrapperCache);
    NSPointerFunctionsOptions keyOptions = NSPointerFunctionsOpaqueMemory | NSPointerFunctionsOpaquePersonality;
    NSPointerFunctionsOptions valueOptions = NSPointerFunctionsWeakMemory | NSPointerFunctionsObjectPersonality;
    globalWrapperCache = [[NSMapTable alloc] initWithKeyOptions:keyOptions valueOptions:valueOptions capacity:0];
}

static NSMapTable *wrapperCache()
{
    if (!globalWrapperCache)
        initWrapperCache();
    return globalWrapperCache;
}

@interface JSVMWrapperCache : NSObject
+ (void)addWrapper:(JSVirtualMachine *)wrapper forJSContextGroupRef:(JSContextGroupRef)group;
+ (JSVirtualMachine *)wrapperForJSContextGroupRef:(JSContextGroupRef)group;
@end

@implementation JSVMWrapperCache

+ (void)addWrapper:(JSVirtualMachine *)wrapper forJSContextGroupRef:(JSContextGroupRef)group
{
    MutexLocker locker(wrapperCacheLock());
    NSMapInsert(wrapperCache(), group, wrapper);
}

+ (JSVirtualMachine *)wrapperForJSContextGroupRef:(JSContextGroupRef)group
{
    MutexLocker locker(wrapperCacheLock());
    return static_cast<JSVirtualMachine *>(NSMapGet(wrapperCache(), group));
}

@end

@implementation JSVirtualMachine {
    JSContextGroupRef m_group;
    NSMapTable *m_contextCache;
    NSMapTable *m_externalObjectGraph;
}

- (id)init
{
    JSContextGroupRef group = JSContextGroupCreate();
    self = [self initWithContextGroupRef:group];
    // The extra JSContextGroupRetain is balanced here.
    JSContextGroupRelease(group);
    return self;
}

- (id)initWithContextGroupRef:(JSContextGroupRef)group
{
    self = [super init];
    if (!self)
        return nil;
    
    m_group = JSContextGroupRetain(group);
    
    NSPointerFunctionsOptions keyOptions = NSPointerFunctionsOpaqueMemory | NSPointerFunctionsOpaquePersonality;
    NSPointerFunctionsOptions valueOptions = NSPointerFunctionsWeakMemory | NSPointerFunctionsObjectPersonality;
    m_contextCache = [[NSMapTable alloc] initWithKeyOptions:keyOptions valueOptions:valueOptions capacity:0];
    
    NSPointerFunctionsOptions weakIDOptions = NSPointerFunctionsWeakMemory | NSPointerFunctionsObjectPersonality;
    NSPointerFunctionsOptions strongIDOptions = NSPointerFunctionsStrongMemory | NSPointerFunctionsObjectPersonality;
    m_externalObjectGraph = [[NSMapTable alloc] initWithKeyOptions:weakIDOptions valueOptions:strongIDOptions capacity:0];
   
    [JSVMWrapperCache addWrapper:self forJSContextGroupRef:group];
 
    return self;
}

- (void)dealloc
{
    JSContextGroupRelease(m_group);
    [m_contextCache release];
    [m_externalObjectGraph release];
    [super dealloc];
}

static id getInternalObjcObject(id object)
{
    if ([object isKindOfClass:[JSManagedValue class]]) {
        JSValue* value = [static_cast<JSManagedValue *>(object) value];
        id temp = tryUnwrapObjcObject([value.context JSGlobalContextRef], [value JSValueRef]);
        if (temp)
            return temp;
        return object;
    }
    
    if ([object isKindOfClass:[JSValue class]]) {
        JSValue *value = static_cast<JSValue *>(object);
        object = tryUnwrapObjcObject([value.context JSGlobalContextRef], [value JSValueRef]);
    }

    return object;
}

- (void)addManagedReference:(id)object withOwner:(id)owner
{    
    object = getInternalObjcObject(object);
    owner = getInternalObjcObject(owner);
    
    if (!object || !owner)
        return;
    
    JSC::APIEntryShim shim(toJS(m_group));
    
    NSMapTable *ownedObjects = [m_externalObjectGraph objectForKey:owner];
    if (!ownedObjects) {
        NSPointerFunctionsOptions weakIDOptions = NSPointerFunctionsWeakMemory | NSPointerFunctionsObjectPersonality;
        NSPointerFunctionsOptions integerOptions = NSPointerFunctionsOpaqueMemory | NSPointerFunctionsIntegerPersonality;
        ownedObjects = [[NSMapTable alloc] initWithKeyOptions:weakIDOptions valueOptions:integerOptions capacity:1];

        [m_externalObjectGraph setObject:ownedObjects forKey:owner];
        [ownedObjects release];
    }
    NSMapInsert(ownedObjects, object, reinterpret_cast<void*>(reinterpret_cast<size_t>(NSMapGet(ownedObjects, object)) + 1));
}

- (void)removeManagedReference:(id)object withOwner:(id)owner
{
    object = getInternalObjcObject(object);
    owner = getInternalObjcObject(owner);
    
    if (!object || !owner)
        return;
    
    JSC::APIEntryShim shim(toJS(m_group));
    
    NSMapTable *ownedObjects = [m_externalObjectGraph objectForKey:owner];
    if (!ownedObjects)
        return;
   
    size_t count = reinterpret_cast<size_t>(NSMapGet(ownedObjects, object));
    if (count > 1) {
        NSMapInsert(ownedObjects, object, reinterpret_cast<void*>(count - 1));
        return;
    }
    
    if (count == 1)
        NSMapRemove(ownedObjects, object);

    if (![ownedObjects count])
        [m_externalObjectGraph removeObjectForKey:owner];
}

@end

@implementation JSVirtualMachine(Internal)

JSContextGroupRef getGroupFromVirtualMachine(JSVirtualMachine *virtualMachine)
{
    return virtualMachine->m_group;
}

+ (JSVirtualMachine *)virtualMachineWithContextGroupRef:(JSContextGroupRef)group
{
    JSVirtualMachine *virtualMachine = [JSVMWrapperCache wrapperForJSContextGroupRef:group];
    if (!virtualMachine)
        virtualMachine = [[[JSVirtualMachine alloc] initWithContextGroupRef:group] autorelease];
    return virtualMachine;
}

- (JSContext *)contextForGlobalContextRef:(JSGlobalContextRef)globalContext
{
    return static_cast<JSContext *>(NSMapGet(m_contextCache, globalContext));
}

- (void)addContext:(JSContext *)wrapper forGlobalContextRef:(JSGlobalContextRef)globalContext
{
    NSMapInsert(m_contextCache, globalContext, wrapper);
}

- (NSMapTable *)externalObjectGraph
{
    return m_externalObjectGraph;
}

@end

void scanExternalObjectGraph(JSC::VM& vm, JSC::SlotVisitor& visitor, void* root)
{
    @autoreleasepool {
        JSVirtualMachine *virtualMachine = [JSVMWrapperCache wrapperForJSContextGroupRef:toRef(&vm)];
        if (!virtualMachine)
            return;
        NSMapTable *externalObjectGraph = [virtualMachine externalObjectGraph];
        Vector<void*> stack;
        stack.append(root);
        while (!stack.isEmpty()) {
            void* nextRoot = stack.last();
            stack.removeLast();
            if (visitor.containsOpaqueRootTriState(nextRoot) == TrueTriState)
                continue;
            visitor.addOpaqueRoot(nextRoot);
            
            NSMapTable *ownedObjects = [externalObjectGraph objectForKey:static_cast<id>(nextRoot)];
            id ownedObject;
            NSEnumerator *enumerator = [ownedObjects keyEnumerator];
            while ((ownedObject = [enumerator nextObject])) {
                ASSERT(reinterpret_cast<size_t>(NSMapGet(ownedObjects, ownedObject)) == 1);
                stack.append(static_cast<void*>(ownedObject));
            }
        }
    }
}

#endif

