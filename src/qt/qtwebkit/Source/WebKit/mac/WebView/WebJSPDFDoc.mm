/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#import "WebJSPDFDoc.h"

#import "WebDataSource.h"
#import "WebDelegateImplementationCaching.h"
#import "WebFrame.h"
#import "WebView.h"
#import <JavaScriptCore/JSObjectRef.h>

static void jsPDFDocInitialize(JSContextRef ctx, JSObjectRef object)
{
    WebDataSource *dataSource = (WebDataSource *)JSObjectGetPrivate(object);
    CFRetain(dataSource);
}

static void jsPDFDocFinalize(JSObjectRef object)
{
    WebDataSource *dataSource = (WebDataSource *)JSObjectGetPrivate(object);
    CFRelease(dataSource);
}

static JSValueRef jsPDFDocPrint(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebDataSource *dataSource = (WebDataSource *)JSObjectGetPrivate(thisObject);

    WebView *webView = [[dataSource webFrame] webView];
    CallUIDelegate(webView, @selector(webView:printFrameView:), [[dataSource webFrame] frameView]);

    return JSValueMakeUndefined(ctx);
}

static JSStaticFunction jsPDFDocStaticFunctions[] = {
    { "print", jsPDFDocPrint, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0 },
};

static JSClassDefinition jsPDFDocClassDefinition = {
    0,
    kJSClassAttributeNone,
    "Doc",
    0,
    0,
    jsPDFDocStaticFunctions,
    jsPDFDocInitialize, jsPDFDocFinalize, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

JSObjectRef makeJSPDFDoc(JSContextRef ctx, WebDataSource *dataSource)
{
    static JSClassRef jsPDFDocClass = JSClassCreate(&jsPDFDocClassDefinition);

    return JSObjectMake(ctx, jsPDFDocClass, dataSource);
}
