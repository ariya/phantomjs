/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#import "WebDataSource.h"
#import "WebDataSourceInternal.h"
#import "WebFrameInternal.h"
#import "WebScriptDebugDelegate.h"
#import "WebScriptDebugger.h"
#import "WebViewInternal.h"
#import <WebCore/Frame.h>
#import <WebCore/ScriptController.h>
#import <WebCore/WebScriptObjectPrivate.h>
#import <WebCore/runtime_root.h>
#import <debugger/Debugger.h>
#import <debugger/DebuggerActivation.h>
#import <debugger/DebuggerCallFrame.h>
#import <interpreter/CallFrame.h>
#import <runtime/Completion.h>
#import <runtime/JSFunction.h>
#import <runtime/JSGlobalObject.h>
#import <runtime/JSLock.h>

using namespace JSC;
using namespace WebCore;

// FIXME: these error strings should be public for future use by WebScriptObject and in WebScriptObject.h
NSString * const WebScriptErrorDomain = @"WebScriptErrorDomain";
NSString * const WebScriptErrorDescriptionKey = @"WebScriptErrorDescription";
NSString * const WebScriptErrorLineNumberKey = @"WebScriptErrorLineNumber";

@interface WebScriptCallFrame (WebScriptDebugDelegateInternal)

- (id)_convertValueToObjcValue:(JSC::JSValue)value;

@end

@interface WebScriptCallFramePrivate : NSObject {
@public
    WebScriptObject        *globalObject;   // the global object's proxy (not retained)
    WebScriptCallFrame     *caller;         // previous stack frame
    DebuggerCallFrame* debuggerCallFrame;
    WebScriptDebugger* debugger;
}
@end

@implementation WebScriptCallFramePrivate
- (void)dealloc
{
    [caller release];
    delete debuggerCallFrame;
    [super dealloc];
}
@end

// WebScriptCallFrame
//
// One of these is created to represent each stack frame.  Additionally, there is a "global"
// frame to represent the outermost scope.  This global frame is always the last frame in
// the chain of callers.
//
// The delegate can assign a "wrapper" to each frame object so it can relay calls through its
// own exported interface.  This class is private to WebCore (and the delegate).

@implementation WebScriptCallFrame (WebScriptDebugDelegateInternal)

- (WebScriptCallFrame *)_initWithGlobalObject:(WebScriptObject *)globalObj debugger:(WebScriptDebugger *)debugger caller:(WebScriptCallFrame *)caller debuggerCallFrame:(const DebuggerCallFrame&)debuggerCallFrame
{
    if ((self = [super init])) {
        _private = [[WebScriptCallFramePrivate alloc] init];
        _private->globalObject = globalObj;
        _private->caller = [caller retain];
        _private->debugger = debugger;
    }
    return self;
}

- (void)_setDebuggerCallFrame:(const DebuggerCallFrame&)debuggerCallFrame
{
    if (!_private->debuggerCallFrame)
        _private->debuggerCallFrame = new DebuggerCallFrame(debuggerCallFrame);
    else
        *_private->debuggerCallFrame = debuggerCallFrame;
}

- (void)_clearDebuggerCallFrame
{
    delete _private->debuggerCallFrame;
    _private->debuggerCallFrame = 0;
}

- (id)_convertValueToObjcValue:(JSC::JSValue)value
{
    if (!value)
        return nil;

    WebScriptObject *globalObject = _private->globalObject;
    if (value == [globalObject _imp])
        return globalObject;

    Bindings::RootObject* root1 = [globalObject _originRootObject];
    if (!root1)
        return nil;

    Bindings::RootObject* root2 = [globalObject _rootObject];
    if (!root2)
        return nil;

    return [WebScriptObject _convertValueToObjcValue:value originRootObject:root1 rootObject:root2];
}

@end



@implementation WebScriptCallFrame

- (void) dealloc
{
    [_userInfo release];
    [_private release];
    [super dealloc];
}

- (void)setUserInfo:(id)userInfo
{
    if (userInfo != _userInfo) {
        [_userInfo release];
        _userInfo = [userInfo retain];
    }
}

- (id)userInfo
{
    return _userInfo;
}

- (WebScriptCallFrame *)caller
{
    return _private->caller;
}

// Returns an array of scope objects (most local first).
// The properties of each scope object are the variables for that scope.
// Note that the last entry in the array will _always_ be the global object (windowScriptObject),
// whose properties are the global variables.

- (NSArray *)scopeChain
{
    if (!_private->debuggerCallFrame)
        return [NSArray array];


    JSScope* scope = _private->debuggerCallFrame->scope();
    JSLockHolder lock(scope->vm());
    if (!scope->next())  // global frame
        return [NSArray arrayWithObject:_private->globalObject];

    NSMutableArray *scopes = [[NSMutableArray alloc] init];

    ScopeChainIterator end = scope->end();
    for (ScopeChainIterator it = scope->begin(); it != end; ++it) {
        JSObject* object = it.get();
        if (object->isActivationObject())
            object = DebuggerActivation::create(*scope->vm(), object);
        [scopes addObject:[self _convertValueToObjcValue:object]];
    }

    NSArray *result = [NSArray arrayWithArray:scopes];
    [scopes release];
    return result;
}

// Returns the name of the function for this frame, if available.
// Returns nil for anonymous functions and for the global frame.

- (NSString *)functionName
{
    if (!_private->debuggerCallFrame)
        return nil;

    String functionName = _private->debuggerCallFrame->functionName();
    return nsStringNilIfEmpty(functionName);
}

// Returns the pending exception for this frame (nil if none).

- (id)exception
{
    if (!_private->debuggerCallFrame)
        return nil;

    JSC::JSValue exception = _private->debuggerCallFrame->exception();
    return exception ? [self _convertValueToObjcValue:exception] : nil;
}

// Evaluate some JavaScript code in the context of this frame.
// The code is evaluated as if by "eval", and the result is returned.
// If there is an (uncaught) exception, it is returned as though _it_ were the result.
// Calling this method on the global frame is not quite the same as calling the WebScriptObject
// method of the same name, due to the treatment of exceptions.

- (id)evaluateWebScript:(NSString *)script
{
    if (!_private->debuggerCallFrame)
        return nil;

    // If this is the global call frame and there is no dynamic global object,
    // Dashcode is attempting to execute JS in the evaluator using a stale
    // WebScriptCallFrame. Instead, we need to set the dynamic global object
    // and evaluate the JS in the global object's global call frame.
    JSGlobalObject* globalObject = _private->debugger->globalObject();
    JSLockHolder lock(globalObject->vm());

    if (self == _private->debugger->globalCallFrame() && !globalObject->vm().dynamicGlobalObject) {
        JSGlobalObject* globalObject = _private->debugger->globalObject();

        DynamicGlobalObjectScope globalObjectScope(globalObject->vm(), globalObject);

        JSC::JSValue exception;
        JSC::JSValue result = evaluateInGlobalCallFrame(script, exception, globalObject);
        if (exception)
            return [self _convertValueToObjcValue:exception];
        return result ? [self _convertValueToObjcValue:result] : nil;        
    }

    JSC::JSValue exception;
    JSC::JSValue result = _private->debuggerCallFrame->evaluate(script, exception);
    if (exception)
        return [self _convertValueToObjcValue:exception];
    return result ? [self _convertValueToObjcValue:result] : nil;
}

@end
