/*
 * Copyright (C) 2004, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#import "DOMInternal.h"

#import "DOMNodeInternal.h"
#import "Frame.h"
#import "JSNode.h"
#import "ScriptController.h"
#import "WebScriptObjectPrivate.h"
#import "runtime_root.h"

//------------------------------------------------------------------------------------------
// Wrapping WebCore implementation objects

static NSMapTable* DOMWrapperCache;

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif    

NSMapTable* createWrapperCache()
{
    // NSMapTable with zeroing weak pointers is the recommended way to build caches like this under garbage collection.
    NSPointerFunctionsOptions keyOptions = NSPointerFunctionsOpaqueMemory | NSPointerFunctionsOpaquePersonality;
    NSPointerFunctionsOptions valueOptions = NSPointerFunctionsZeroingWeakMemory | NSPointerFunctionsObjectPersonality;
    return [[NSMapTable alloc] initWithKeyOptions:keyOptions valueOptions:valueOptions capacity:0];
}

#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

NSObject* getDOMWrapper(DOMObjectInternal* impl)
{
    if (!DOMWrapperCache)
        return nil;
    return static_cast<NSObject*>(NSMapGet(DOMWrapperCache, impl));
}

void addDOMWrapper(NSObject* wrapper, DOMObjectInternal* impl)
{
    if (!DOMWrapperCache)
        DOMWrapperCache = createWrapperCache();
    NSMapInsert(DOMWrapperCache, impl, wrapper);
}

void removeDOMWrapper(DOMObjectInternal* impl)
{
    if (!DOMWrapperCache)
        return;
    NSMapRemove(DOMWrapperCache, impl);
}

//------------------------------------------------------------------------------------------

@implementation WebScriptObject (WebScriptObjectInternal)

// Only called by DOMObject subclass.
- (id)_init
{
    self = [super init];

    if (![self isKindOfClass:[DOMObject class]]) {
        [NSException raise:NSGenericException format:@"+%@: _init is an internal initializer", [self class]];
        return nil;
    }

    _private = [[WebScriptObjectPrivate alloc] init];
    _private->isCreatedByDOMWrapper = YES;
    
    return self;
}

- (void)_initializeScriptDOMNodeImp
{
    ASSERT(_private->isCreatedByDOMWrapper);
    
    if (![self isKindOfClass:[DOMNode class]]) {
        // DOMObject can't map back to a document, and thus an interpreter,
        // so for now only create wrappers for DOMNodes.
        NSLog(@"%s:%d:  We don't know how to create ObjC JS wrappers from DOMObjects yet.", __FILE__, __LINE__);
        return;
    }
    
    // Extract the WebCore::Node from the ObjectiveC wrapper.
    DOMNode *n = (DOMNode *)self;
    WebCore::Node *nodeImpl = core(n);

    // Dig up Interpreter and ExecState.
    WebCore::Frame *frame = 0;
    if (WebCore::Document* document = nodeImpl->document())
        frame = document->frame();
    if (!frame)
        return;

    // The global object which should own this node - FIXME: does this need to be isolated-world aware?
    WebCore::JSDOMGlobalObject* globalObject = frame->script()->globalObject(WebCore::mainThreadNormalWorld());
    JSC::ExecState *exec = globalObject->globalExec();

    // Get (or create) a cached JS object for the DOM node.
    JSC::JSObject *scriptImp = asObject(WebCore::toJS(exec, globalObject, nodeImpl));

    JSC::Bindings::RootObject* rootObject = frame->script()->bindingRootObject();

    [self _setImp:scriptImp originRootObject:rootObject rootObject:rootObject];
}

@end
