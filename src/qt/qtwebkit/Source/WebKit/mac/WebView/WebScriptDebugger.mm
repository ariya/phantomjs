/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#import "WebScriptDebugger.h"

#import "WebDelegateImplementationCaching.h"
#import "WebFrameInternal.h"
#import "WebScriptDebugDelegate.h"
#import "WebViewInternal.h"
#import <JavaScriptCore/DebuggerCallFrame.h>
#import <JavaScriptCore/JSGlobalObject.h>
#import <JavaScriptCore/SourceProvider.h>
#import <JavaScriptCore/StrongInlines.h>
#import <WebCore/DOMWindow.h>
#import <WebCore/Frame.h>
#import <WebCore/JSDOMWindow.h>
#import <WebCore/KURL.h>
#import <WebCore/ScriptController.h>

using namespace JSC;
using namespace WebCore;

@interface WebScriptCallFrame (WebScriptDebugDelegateInternal)
- (WebScriptCallFrame *)_initWithGlobalObject:(WebScriptObject *)globalObj debugger:(WebScriptDebugger *)debugger caller:(WebScriptCallFrame *)caller debuggerCallFrame:(const DebuggerCallFrame&)debuggerCallFrame;
- (void)_setDebuggerCallFrame:(const DebuggerCallFrame&)debuggerCallFrame;
- (void)_clearDebuggerCallFrame;
@end

static NSString *toNSString(SourceProvider* sourceProvider)
{
    const String& sourceString = sourceProvider->source();
    if (sourceString.isEmpty())
        return nil;
    return sourceString;
}

// Convert String to NSURL.
static NSURL *toNSURL(const String& s)
{
    if (s.isEmpty())
        return nil;
    return KURL(ParsedURLString, s);
}

static WebFrame *toWebFrame(JSGlobalObject* globalObject)
{
    JSDOMWindow* window = static_cast<JSDOMWindow*>(globalObject);
    return kit(window->impl()->frame());
}

WebScriptDebugger::WebScriptDebugger(JSGlobalObject* globalObject)
    : m_callingDelegate(false)
    , m_globalObject(globalObject->vm(), globalObject)
{
    attach(globalObject);
    initGlobalCallFrame(globalObject->globalExec());
}

void WebScriptDebugger::initGlobalCallFrame(const DebuggerCallFrame& debuggerCallFrame)
{
    m_callingDelegate = true;

    WebFrame *webFrame = toWebFrame(debuggerCallFrame.dynamicGlobalObject());

    m_topCallFrame = adoptNS([[WebScriptCallFrame alloc] _initWithGlobalObject:core(webFrame)->script()->windowScriptObject() debugger:this caller:m_topCallFrame.get() debuggerCallFrame:debuggerCallFrame]);
    m_globalCallFrame = m_topCallFrame;

    WebView *webView = [webFrame webView];
    WebScriptDebugDelegateImplementationCache* implementations = WebViewGetScriptDebugDelegateImplementations(webView);
    if (implementations->didEnterCallFrameFunc)
        CallScriptDebugDelegate(implementations->didEnterCallFrameFunc, webView, @selector(webView:didEnterCallFrame:sourceId:line:forWebFrame:), m_topCallFrame.get(), static_cast<NSInteger>(0), -1, webFrame);

    m_callingDelegate = false;
}

// callbacks - relay to delegate
void WebScriptDebugger::sourceParsed(ExecState* exec, SourceProvider* sourceProvider, int errorLine, const String& errorMsg)
{
    if (m_callingDelegate)
        return;

    m_callingDelegate = true;

    NSString *nsSource = toNSString(sourceProvider);
    NSURL *nsURL = toNSURL(sourceProvider->url());
    int firstLine = sourceProvider->startPosition().m_line.oneBasedInt();

    WebFrame *webFrame = toWebFrame(exec->dynamicGlobalObject());
    WebView *webView = [webFrame webView];
    WebScriptDebugDelegateImplementationCache* implementations = WebViewGetScriptDebugDelegateImplementations(webView);

    if (errorLine == -1) {
        if (implementations->didParseSourceFunc) {
            if (implementations->didParseSourceExpectsBaseLineNumber)
                CallScriptDebugDelegate(implementations->didParseSourceFunc, webView, @selector(webView:didParseSource:baseLineNumber:fromURL:sourceId:forWebFrame:), nsSource, firstLine, nsURL, sourceProvider->asID(), webFrame);
            else
                CallScriptDebugDelegate(implementations->didParseSourceFunc, webView, @selector(webView:didParseSource:fromURL:sourceId:forWebFrame:), nsSource, [nsURL absoluteString], sourceProvider->asID(), webFrame);
        }
    } else {
        NSString* nsErrorMessage = nsStringNilIfEmpty(errorMsg);
        NSDictionary *info = [[NSDictionary alloc] initWithObjectsAndKeys:nsErrorMessage, WebScriptErrorDescriptionKey, [NSNumber numberWithUnsignedInt:errorLine], WebScriptErrorLineNumberKey, nil];
        NSError *error = [[NSError alloc] initWithDomain:WebScriptErrorDomain code:WebScriptGeneralErrorCode userInfo:info];

        if (implementations->failedToParseSourceFunc)
            CallScriptDebugDelegate(implementations->failedToParseSourceFunc, webView, @selector(webView:failedToParseSource:baseLineNumber:fromURL:withError:forWebFrame:), nsSource, firstLine, nsURL, error, webFrame);

        [error release];
        [info release];
    }

    m_callingDelegate = false;
}

void WebScriptDebugger::callEvent(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber, int columnNumber)
{
    if (m_callingDelegate)
        return;

    m_callingDelegate = true;

    WebFrame *webFrame = toWebFrame(debuggerCallFrame.dynamicGlobalObject());

    m_topCallFrame = adoptNS([[WebScriptCallFrame alloc] _initWithGlobalObject:core(webFrame)->script()->windowScriptObject() debugger:this caller:m_topCallFrame.get() debuggerCallFrame:debuggerCallFrame]);

    WebView *webView = [webFrame webView];
    WebScriptDebugDelegateImplementationCache* implementations = WebViewGetScriptDebugDelegateImplementations(webView);
    if (implementations->didEnterCallFrameFunc)
        CallScriptDebugDelegate(implementations->didEnterCallFrameFunc, webView, @selector(webView:didEnterCallFrame:sourceId:line:forWebFrame:), m_topCallFrame.get(), sourceID, lineNumber, webFrame);

    m_callingDelegate = false;
}

void WebScriptDebugger::atStatement(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber, int columnNumber)
{
    if (m_callingDelegate)
        return;

    m_callingDelegate = true;

    WebFrame *webFrame = toWebFrame(debuggerCallFrame.dynamicGlobalObject());
    WebView *webView = [webFrame webView];

    [m_topCallFrame.get() _setDebuggerCallFrame:debuggerCallFrame];

    WebScriptDebugDelegateImplementationCache* implementations = WebViewGetScriptDebugDelegateImplementations(webView);
    if (implementations->willExecuteStatementFunc)
        CallScriptDebugDelegate(implementations->willExecuteStatementFunc, webView, @selector(webView:willExecuteStatement:sourceId:line:forWebFrame:), m_topCallFrame.get(), sourceID, lineNumber, webFrame);

    m_callingDelegate = false;
}

void WebScriptDebugger::returnEvent(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber, int columnNumber)
{
    if (m_callingDelegate)
        return;

    m_callingDelegate = true;

    WebFrame *webFrame = toWebFrame(debuggerCallFrame.dynamicGlobalObject());
    WebView *webView = [webFrame webView];

    [m_topCallFrame.get() _setDebuggerCallFrame:debuggerCallFrame];

    WebScriptDebugDelegateImplementationCache* implementations = WebViewGetScriptDebugDelegateImplementations(webView);
    if (implementations->willLeaveCallFrameFunc)
        CallScriptDebugDelegate(implementations->willLeaveCallFrameFunc, webView, @selector(webView:willLeaveCallFrame:sourceId:line:forWebFrame:), m_topCallFrame.get(), sourceID, lineNumber, webFrame);

    [m_topCallFrame.get() _clearDebuggerCallFrame];
    m_topCallFrame = [m_topCallFrame.get() caller];

    m_callingDelegate = false;
}

void WebScriptDebugger::exception(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber, int columnNumber, bool hasHandler)
{
    if (m_callingDelegate)
        return;

    m_callingDelegate = true;

    WebFrame *webFrame = toWebFrame(debuggerCallFrame.dynamicGlobalObject());
    WebView *webView = [webFrame webView];
    [m_topCallFrame.get() _setDebuggerCallFrame:debuggerCallFrame];

    WebScriptDebugDelegateImplementationCache* cache = WebViewGetScriptDebugDelegateImplementations(webView);
    if (cache->exceptionWasRaisedFunc) {
        if (cache->exceptionWasRaisedExpectsHasHandlerFlag)
            CallScriptDebugDelegate(cache->exceptionWasRaisedFunc, webView, @selector(webView:exceptionWasRaised:hasHandler:sourceId:line:forWebFrame:), m_topCallFrame.get(), hasHandler, sourceID, lineNumber, webFrame);
        else
            CallScriptDebugDelegate(cache->exceptionWasRaisedFunc, webView, @selector(webView:exceptionWasRaised:sourceId:line:forWebFrame:), m_topCallFrame.get(), sourceID, lineNumber, webFrame);
    }

    m_callingDelegate = false;
}

void WebScriptDebugger::willExecuteProgram(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineno, int columnno)
{
    callEvent(debuggerCallFrame, sourceID, lineno, columnno);
}

void WebScriptDebugger::didExecuteProgram(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineno, int columnno)
{
    returnEvent(debuggerCallFrame, sourceID, lineno, columnno);
}

void WebScriptDebugger::didReachBreakpoint(const DebuggerCallFrame&, intptr_t, int, int)
{
    return;
}
