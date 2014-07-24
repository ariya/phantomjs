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

#ifndef JSContextInternal_h
#define JSContextInternal_h

#import <JavaScriptCore/JavaScriptCore.h>

#if JSC_OBJC_API_ENABLED

#import <JavaScriptCore/JSContext.h>

struct CallbackData {
    CallbackData *next;
    JSContext *context;
    JSValue *preservedException;
    JSValueRef thisValue;
    JSValue *currentThis;
    size_t argumentCount;
    const JSValueRef *arguments;
    NSArray *currentArguments;
};

class WeakContextRef {
public:
    WeakContextRef(JSContext * = nil);
    ~WeakContextRef();

    JSContext * get();
    void set(JSContext *);

private:
    JSContext *m_weakContext;
};

@class JSWrapperMap;

@interface JSContext(Internal)

- (id)initWithGlobalContextRef:(JSGlobalContextRef)context;

- (void)notifyException:(JSValueRef)exception;
- (JSValue *)valueFromNotifyException:(JSValueRef)exception;
- (BOOL)boolFromNotifyException:(JSValueRef)exception;

- (void)beginCallbackWithData:(CallbackData *)callbackData thisValue:(JSValueRef)thisValue argumentCount:(size_t)argumentCount arguments:(const JSValueRef *)arguments;
- (void)endCallbackWithData:(CallbackData *)callbackData;

- (JSValue *)wrapperForObjCObject:(id)object;
- (JSValue *)wrapperForJSObject:(JSValueRef)value;

@property (readonly, retain) JSWrapperMap *wrapperMap;

@end

#endif

#endif // JSContextInternal_h
