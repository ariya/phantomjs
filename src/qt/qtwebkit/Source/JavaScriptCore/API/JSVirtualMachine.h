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

#import <JavaScriptCore/JavaScriptCore.h>

#if JSC_OBJC_API_ENABLED

// An instance of JSVirtualMachine represents a single JavaScript "object space"
// or set of execution resources. Thread safety is supported by locking the
// virtual machine, with concurrent JavaScript execution supported by allocating
// separate instances of JSVirtualMachine.

NS_CLASS_AVAILABLE(10_9, NA)
@interface JSVirtualMachine : NSObject

// Create a new JSVirtualMachine.
- (id)init;

// addManagedReference:withOwner and removeManagedReference:withOwner allow 
// clients of JSVirtualMachine to make the JavaScript runtime aware of 
// arbitrary external Objective-C object graphs. The runtime can then use 
// this information to retain any JavaScript values that are referenced 
// from somewhere in said object graph.
// 
// For correct behavior clients must make their external object graphs 
// reachable from within the JavaScript runtime. If an Objective-C object is 
// reachable from within the JavaScript runtime, all managed references 
// transitively reachable from it as recorded with 
// addManagedReference:withOwner: will be scanned by the garbage collector.
// 
- (void)addManagedReference:(id)object withOwner:(id)owner;
- (void)removeManagedReference:(id)object withOwner:(id)owner;

@end

#endif
