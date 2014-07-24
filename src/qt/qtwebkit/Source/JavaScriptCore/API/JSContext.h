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

#ifndef JSContext_h
#define JSContext_h

#include <JavaScriptCore/JavaScript.h>

#if JSC_OBJC_API_ENABLED

@class JSVirtualMachine, JSValue;

// An instance of JSContext represents a JavaScript execution environment. All
// JavaScript execution takes place within a context.
// JSContext is also used to manage the life-cycle of objects within the
// JavaScript virtual machine. Every instance of JSValue is associated with a
// JSContext via a strong reference. The JSValue will keep the JSContext it
// references alive so long as the JSValue remains alive. When all of the JSValues
// that reference a particular JSContext have been deallocated the JSContext 
// will be deallocated unless it has been previously retained.

NS_CLASS_AVAILABLE(10_9, NA)
@interface JSContext : NSObject

// Create a JSContext.
- (id)init;
// Create a JSContext in the specified virtual machine.
- (id)initWithVirtualMachine:(JSVirtualMachine *)virtualMachine;

// Evaluate a string of JavaScript code.
- (JSValue *)evaluateScript:(NSString *)script;

// This method retrieves the global object of the JavaScript execution context.
// Instances of JSContext originating from WebKit will return a reference to the
// WindowProxy object.
- (JSValue *)globalObject;

// This method may be called from within an Objective-C block or method invoked
// as a callback from JavaScript to retrieve the callback's context. Outside of
// a callback from JavaScript this method will return nil.
+ (JSContext *)currentContext;
// This method may be called from within an Objective-C block or method invoked
// as a callback from JavaScript to retrieve the callback's this value. Outside
// of a callback from JavaScript this method will return nil.
+ (JSValue *)currentThis;
// This method may be called from within an Objective-C block or method invoked
// as a callback from JavaScript to retrieve the callback's arguments, objects
// in the returned array are instances of JSValue. Outside of a callback from
// JavaScript this method will return nil.
+ (NSArray *)currentArguments;

// The "exception" property may be used to throw an exception to JavaScript.
// Before a callback is made from JavaScript to an Objective-C block or method,
// the prior value of the exception property will be preserved and the property
// will be set to nil. After the callback has completed the new value of the
// exception property will be read, and prior value restored. If the new value
// of exception is not nil, the callback will result in that value being thrown.
// This property may also be used to check for uncaught exceptions arising from
// API function calls (since the default behaviour of "exceptionHandler" is to
// assign an uncaught exception to this property).
// If a JSValue originating from a different JSVirtualMachine than this context
// is assigned to this property, an Objective-C exception will be raised.
@property(retain) JSValue *exception;

// If a call to an API function results in an uncaught JavaScript exception, the
// "exceptionHandler" block will be invoked. The default implementation for the
// exception handler will store the exception to the exception property on
// context. As a consequence the default behaviour is for unhandled exceptions
// occurring within a callback from JavaScript to be rethrown upon return.
// Setting this value to nil will result in all uncaught exceptions thrown from
// the API being silently consumed.
@property(copy) void(^exceptionHandler)(JSContext *context, JSValue *exception);

// All instances of JSContext are associated with a single JSVirtualMachine. The
// virtual machine provides an "object space" or set of execution resources.
@property(readonly, retain) JSVirtualMachine *virtualMachine;

@end

// Instances of JSContext implement the following methods in order to enable
// support for subscript access by key and index, for example:
//
//    JSContext *context;
//    JSValue *v = context[@"X"]; // Get value for "X" from the global object.
//    context[@"Y"] = v;          // Assign 'v' to "Y" on the global object.
//
// An object key passed as a subscript will be converted to a JavaScript value,
// and then the value converted to a string used to resolve a property of the
// global object.
@interface JSContext(SubscriptSupport)

- (JSValue *)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(NSObject <NSCopying> *)key;

@end

// These functions are for bridging between the C API and the Objective-C API.
@interface JSContext(JSContextRefSupport)
// Creates a JSContext, wrapping its C API counterpart.
+ (JSContext *)contextWithJSGlobalContextRef:(JSGlobalContextRef)jsGlobalContextRef;
// Returns the C API counterpart wrapped by a JSContext.
- (JSGlobalContextRef)JSGlobalContextRef;
@end

#endif

#endif // JSContext_h
