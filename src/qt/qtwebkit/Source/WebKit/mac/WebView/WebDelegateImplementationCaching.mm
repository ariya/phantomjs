/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 David Smith (catfish.man@gmail.com)
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

#import "WebDelegateImplementationCaching.h"

#import "WebKitLogging.h"
#import "WebView.h"
#import "WebViewData.h"
#import <wtf/ObjcRuntimeExtras.h>

@implementation WebView (WebDelegateImplementationCaching)

WebResourceDelegateImplementationCache* WebViewGetResourceLoadDelegateImplementations(WebView *webView)
{
    static WebResourceDelegateImplementationCache empty;
    if (!webView)
        return &empty;
    return &webView->_private->resourceLoadDelegateImplementations;
}

WebFrameLoadDelegateImplementationCache* WebViewGetFrameLoadDelegateImplementations(WebView *webView)
{
    static WebFrameLoadDelegateImplementationCache empty;
    if (!webView)
        return &empty;
    return &webView->_private->frameLoadDelegateImplementations;
}

WebScriptDebugDelegateImplementationCache* WebViewGetScriptDebugDelegateImplementations(WebView *webView)
{
    static WebScriptDebugDelegateImplementationCache empty;
    if (!webView)
        return &empty;
    return &webView->_private->scriptDebugDelegateImplementations;
}

WebHistoryDelegateImplementationCache* WebViewGetHistoryDelegateImplementations(WebView *webView)
{
    static WebHistoryDelegateImplementationCache empty;
    if (!webView)
        return &empty;
    return &webView->_private->historyDelegateImplementations;
}

// We use these functions to call the delegates and block exceptions. These functions are
// declared inside a WebView category to get direct access to the delegate data memebers,
// preventing more ObjC message dispatch and compensating for the expense of the @try/@catch.

typedef float (*ObjCMsgSendFPRet)(id, SEL, ...);
#if defined(__i386__)
static const ObjCMsgSendFPRet objc_msgSend_float_return = reinterpret_cast<ObjCMsgSendFPRet>(objc_msgSend_fpret);
#else
static const ObjCMsgSendFPRet objc_msgSend_float_return = reinterpret_cast<ObjCMsgSendFPRet>(objc_msgSend);
#endif

static inline id CallDelegate(WebView *self, id delegate, SEL selector)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, self);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(WebView *self, id delegate, SEL selector, id object)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, self, object);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(WebView *self, id delegate, SEL selector, NSRect rect)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, self, rect);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(WebView *self, id delegate, SEL selector, id object1, id object2)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, self, object1, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(WebView *self, id delegate, SEL selector, id object, BOOL boolean)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, self, object, boolean);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(WebView *self, id delegate, SEL selector, id object1, id object2, id object3)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, self, object1, object2, object3);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(WebView *self, id delegate, SEL selector, id object, NSUInteger integer)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, self, object, integer);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline float CallDelegateReturningFloat(WebView *self, id delegate, SEL selector)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return 0.0f;
    @try {
        return objc_msgSend_float_return(delegate, selector, self);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return 0.0f;
}

static inline BOOL CallDelegateReturningBoolean(BOOL result, WebView *self, id delegate, SEL selector)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return result;
    @try {
        return wtfObjcMsgSend<BOOL>(delegate, selector, self);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

static inline BOOL CallDelegateReturningBoolean(BOOL result, WebView *self, id delegate, SEL selector, id object)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return result;
    @try {
        return wtfObjcMsgSend<BOOL>(delegate, selector, self, object);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

static inline BOOL CallDelegateReturningBoolean(BOOL result, WebView *self, id delegate, SEL selector, id object, BOOL boolean)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return result;
    @try {
        return wtfObjcMsgSend<BOOL>(delegate, selector, self, object, boolean);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

static inline BOOL CallDelegateReturningBoolean(BOOL result, WebView *self, id delegate, SEL selector, id object, BOOL boolean, id object2)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return result;
    @try {
        return wtfObjcMsgSend<BOOL>(delegate, selector, self, object, boolean, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

static inline BOOL CallDelegateReturningBoolean(BOOL result, WebView *self, id delegate, SEL selector, id object1, id object2)
{
    if (!delegate || ![delegate respondsToSelector:selector])
        return result;
    @try {
        return wtfObjcMsgSend<BOOL>(delegate, selector, self, object1, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, NSUInteger integer)
{
    if (!delegate)
        return nil;
    @try {
        return implementation(delegate, selector, self, integer);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, id object2)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, id object2, id object3)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, object2, object3);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, id object2, id object3, id object4)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, object2, object3, object4);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, NSInteger integer, id object2)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, integer, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, NSInteger integer1, int integer2, id object2)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, integer1, integer2, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, BOOL boolean, NSInteger integer1, int integer2, id object2)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, boolean, integer1, integer2, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, id object2, NSInteger integer, id object3)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, object2, integer, object3);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, NSInteger integer1, id object2, NSInteger integer2, id object3)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, integer1, object2, integer2, object3);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, NSInteger integer, id object2, id object3, id object4)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, integer, object2, object3, object4);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

static inline id CallDelegate(IMP implementation, WebView *self, id delegate, SEL selector, id object1, NSTimeInterval interval, id object2, id object3)
{
    if (!delegate)
        return nil;
    @try {
        return wtfCallIMP<id>(implementation, delegate, selector, self, object1, interval, object2, object3);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

id CallUIDelegate(WebView *self, SEL selector)
{
    return CallDelegate(self, self->_private->UIDelegate, selector);
}

id CallUIDelegate(WebView *self, SEL selector, id object)
{
    return CallDelegate(self, self->_private->UIDelegate, selector, object);
}

id CallUIDelegate(WebView *self, SEL selector, id object, BOOL boolean)
{
    return CallDelegate(self, self->_private->UIDelegate, selector, object, boolean);
}

id CallUIDelegate(WebView *self, SEL selector, NSRect rect)
{
    return CallDelegate(self, self->_private->UIDelegate, selector, rect);
}

id CallUIDelegate(WebView *self, SEL selector, id object1, id object2)
{
    return CallDelegate(self, self->_private->UIDelegate, selector, object1, object2);
}

id CallUIDelegate(WebView *self, SEL selector, id object1, id object2, id object3)
{
    return CallDelegate(self, self->_private->UIDelegate, selector, object1, object2, object3);
}

id CallUIDelegate(WebView *self, SEL selector, id object, NSUInteger integer)
{
    return CallDelegate(self, self->_private->UIDelegate, selector, object, integer);
}

float CallUIDelegateReturningFloat(WebView *self, SEL selector)
{
    return CallDelegateReturningFloat(self, self->_private->UIDelegate, selector);
}

BOOL CallUIDelegateReturningBoolean(BOOL result, WebView *self, SEL selector)
{
    return CallDelegateReturningBoolean(result, self, self->_private->UIDelegate, selector);
}

BOOL CallUIDelegateReturningBoolean(BOOL result, WebView *self, SEL selector, id object)
{
    return CallDelegateReturningBoolean(result, self, self->_private->UIDelegate, selector, object);
}

BOOL CallUIDelegateReturningBoolean(BOOL result, WebView *self, SEL selector, id object, BOOL boolean)
{
    return CallDelegateReturningBoolean(result, self, self->_private->UIDelegate, selector, object, boolean);
}

BOOL CallUIDelegateReturningBoolean(BOOL result, WebView *self, SEL selector, id object, BOOL boolean, id object2)
{
    return CallDelegateReturningBoolean(result, self, self->_private->UIDelegate, selector, object, boolean, object2);
}

BOOL CallUIDelegateReturningBoolean(BOOL result, WebView *self, SEL selector, id object1, id object2)
{
    return CallDelegateReturningBoolean(result, self, self->_private->UIDelegate, selector, object1, object2);
}

id CallFrameLoadDelegate(IMP implementation, WebView *self, SEL selector)
{
    return CallDelegate(implementation, self, self->_private->frameLoadDelegate, selector);
}

id CallFrameLoadDelegate(IMP implementation, WebView *self, SEL selector, NSUInteger integer)
{
    return CallDelegate(implementation, self, self->_private->frameLoadDelegate, selector, integer);
}

id CallFrameLoadDelegate(IMP implementation, WebView *self, SEL selector, id object)
{
    return CallDelegate(implementation, self, self->_private->frameLoadDelegate, selector, object);
}

id CallFrameLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2)
{
    return CallDelegate(implementation, self, self->_private->frameLoadDelegate, selector, object1, object2);
}

id CallFrameLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2, id object3)
{
    return CallDelegate(implementation, self, self->_private->frameLoadDelegate, selector, object1, object2, object3);
}

id CallFrameLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2, id object3, id object4)
{
    return CallDelegate(implementation, self, self->_private->frameLoadDelegate, selector, object1, object2, object3, object4);
}

id CallFrameLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, NSTimeInterval interval, id object2, id object3)
{
    return CallDelegate(implementation, self, self->_private->frameLoadDelegate, selector, object1, interval, object2, object3);
}

BOOL CallFrameLoadDelegateReturningBoolean(BOOL result, IMP implementation, WebView *self, SEL selector)
{
    @try {
        return reinterpret_cast<BOOL (*)(id, SEL, WebView *)>(objc_msgSend)(self->_private->frameLoadDelegate, selector, self);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

id CallResourceLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2)
{
    return CallDelegate(implementation, self, self->_private->resourceProgressDelegate, selector, object1, object2);
}

id CallResourceLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2, id object3)
{
    return CallDelegate(implementation, self, self->_private->resourceProgressDelegate, selector, object1, object2, object3);
}

id CallResourceLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2, id object3, id object4)
{
    return CallDelegate(implementation, self, self->_private->resourceProgressDelegate, selector, object1, object2, object3, object4);
}

id CallResourceLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, NSInteger integer, id object2)
{
    return CallDelegate(implementation, self, self->_private->resourceProgressDelegate, selector, object1, integer, object2);
}

id CallResourceLoadDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2, NSInteger integer, id object3)
{
    return CallDelegate(implementation, self, self->_private->resourceProgressDelegate, selector, object1, object2, integer, object3);
}

BOOL CallResourceLoadDelegateReturningBoolean(BOOL result, IMP implementation, WebView *self, SEL selector, id object1)
{
    @try {
        return wtfObjcMsgSend<BOOL>(self->_private->resourceProgressDelegate, selector, self, object1);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

BOOL CallResourceLoadDelegateReturningBoolean(BOOL result, IMP implementation, WebView *self, SEL selector, id object1, id object2)
{
    @try {
        return wtfObjcMsgSend<BOOL>(self->_private->resourceProgressDelegate, selector, self, object1, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

BOOL CallResourceLoadDelegateReturningBoolean(BOOL result, IMP implementation, WebView *self, SEL selector, id object1, id object2, id object3)
{
    @try {
        return wtfObjcMsgSend<BOOL>(self->_private->resourceProgressDelegate, selector, self, object1, object2, object3);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

id CallScriptDebugDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2, NSInteger integer, id object3)
{
    return CallDelegate(implementation, self, self->_private->scriptDebugDelegate, selector, object1, object2, integer, object3);
}

id CallScriptDebugDelegate(IMP implementation, WebView *self, SEL selector, id object1, NSInteger integer1, id object2, NSInteger integer2, id object3)
{
    return CallDelegate(implementation, self, self->_private->scriptDebugDelegate, selector, object1, integer1, object2, integer2, object3);
}

id CallScriptDebugDelegate(IMP implementation, WebView *self, SEL selector, id object1, NSInteger integer, id object2, id object3, id object4)
{
    return CallDelegate(implementation, self, self->_private->scriptDebugDelegate, selector, object1, integer, object2, object3, object4);
}

id CallScriptDebugDelegate(IMP implementation, WebView *self, SEL selector, id object1, NSInteger integer1, int integer2, id object2)
{
    return CallDelegate(implementation, self, self->_private->scriptDebugDelegate, selector, object1, integer1, integer2, object2);
}

id CallScriptDebugDelegate(IMP implementation, WebView *self, SEL selector, id object1, BOOL boolean, NSInteger integer1, int integer2, id object2)
{
    return CallDelegate(implementation, self, self->_private->scriptDebugDelegate, selector, object1, boolean, integer1, integer2, object2);
}

id CallHistoryDelegate(IMP implementation, WebView *self, SEL selector)
{
    return CallDelegate(implementation, self, self->_private->historyDelegate, selector);
}

id CallHistoryDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2)
{
    return CallDelegate(implementation, self, self->_private->historyDelegate, selector, object1, object2);
}

id CallHistoryDelegate(IMP implementation, WebView *self, SEL selector, id object1, id object2, id object3)
{
    return CallDelegate(implementation, self, self->_private->historyDelegate, selector, object1, object2, object3);
}

// The form delegate needs to have it's own implementation, because the first argument is never the WebView

id CallFormDelegate(WebView *self, SEL selector, id object1, id object2)
{
    id delegate = self->_private->formDelegate;
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, object1, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

id CallFormDelegate(WebView *self, SEL selector, id object1, id object2, id object3)
{
    id delegate = self->_private->formDelegate;
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, object1, object2, object3);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

id CallFormDelegate(WebView *self, SEL selector, id object1, id object2, id object3, id object4, id object5)
{
    id delegate = self->_private->formDelegate;
    if (!delegate || ![delegate respondsToSelector:selector])
        return nil;
    @try {
        return wtfObjcMsgSend<id>(delegate, selector, object1, object2, object3, object4, object5);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return nil;
}

BOOL CallFormDelegateReturningBoolean(BOOL result, WebView *self, SEL selector, id object1, SEL selectorArg, id object2)
{
    id delegate = self->_private->formDelegate;
    if (!delegate || ![delegate respondsToSelector:selector])
        return result;
    @try {
        return wtfObjcMsgSend<BOOL>(delegate, selector, object1, selectorArg, object2);
    } @catch(id exception) {
        ReportDiscardedDelegateException(selector, exception);
    }
    return result;
}

@end
