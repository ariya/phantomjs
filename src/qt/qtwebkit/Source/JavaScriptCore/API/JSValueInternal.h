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

#ifndef JSValueInternal_h
#define JSValueInternal_h

#import <JavaScriptCore/JavaScriptCore.h>
#import <JavaScriptCore/JSValue.h>

#if JSC_OBJC_API_ENABLED

@interface JSValue(Internal)

JSValueRef valueInternalValue(JSValue *);

- (JSValue *)initWithValue:(JSValueRef)value inContext:(JSContext *)context;

JSValueRef objectToValue(JSContext *, id);
id valueToObject(JSContext *, JSValueRef);
id valueToNumber(JSGlobalContextRef, JSValueRef, JSValueRef* exception);
id valueToString(JSGlobalContextRef, JSValueRef, JSValueRef* exception);
id valueToDate(JSGlobalContextRef, JSValueRef, JSValueRef* exception);
id valueToArray(JSGlobalContextRef, JSValueRef, JSValueRef* exception);
id valueToDictionary(JSGlobalContextRef, JSValueRef, JSValueRef* exception);

+ (SEL)selectorForStructToValue:(const char *)structTag;
+ (SEL)selectorForValueToStruct:(const char *)structTag;

@end

NSInvocation *typeToValueInvocationFor(const char* encodedType);
NSInvocation *valueToTypeInvocationFor(const char* encodedType);

#endif

#endif // JSValueInternal_h
