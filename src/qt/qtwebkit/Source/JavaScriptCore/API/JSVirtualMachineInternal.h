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

#ifndef JSVirtualMachineInternal_h
#define JSVirtualMachineInternal_h

#import <JavaScriptCore/JavaScriptCore.h>

#if JSC_OBJC_API_ENABLED

namespace JSC {
class VM;
class SlotVisitor;
}

#if defined(__OBJC__)
@interface JSVirtualMachine(Internal)

JSContextGroupRef getGroupFromVirtualMachine(JSVirtualMachine *);

+ (JSVirtualMachine *)virtualMachineWithContextGroupRef:(JSContextGroupRef)group;

- (JSContext *)contextForGlobalContextRef:(JSGlobalContextRef)globalContext;
- (void)addContext:(JSContext *)wrapper forGlobalContextRef:(JSGlobalContextRef)globalContext;

- (NSMapTable *)externalObjectGraph;

@end
#endif // defined(__OBJC__)

void scanExternalObjectGraph(JSC::VM&, JSC::SlotVisitor&, void* root);

#endif

#endif // JSVirtualMachineInternal_h
