/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "QtBuiltinBundlePage.h"

#include "QtBuiltinBundle.h"
#include "WKArray.h"
#include "WKBundleFrame.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "WKStringPrivate.h"
#include "WKStringQt.h"
#include <JavaScript.h>
#include <JavaScriptCore/JSRetainPtr.h>

namespace WebKit {

QtBuiltinBundlePage::QtBuiltinBundlePage(QtBuiltinBundle* bundle, WKBundlePageRef page)
    : m_bundle(bundle)
    , m_page(page)
    , m_navigatorQtObject(0)
    , m_navigatorQtObjectEnabled(false)
{
    WKBundlePageLoaderClient loaderClient = {
        kWKBundlePageLoaderClientCurrentVersion,
        this,
        0, // didStartProvisionalLoadForFrame
        0, // didReceiveServerRedirectForProvisionalLoadForFrame
        0, // didFailProvisionalLoadWithErrorForFrame
        0, // didCommitLoadForFrame
        0, // didFinishDocumentLoadForFrame
        0, // didFinishLoadForFrame
        0, // didFailLoadWithErrorForFrame
        0, // didSameDocumentNavigationForFrame
        0, // didReceiveTitleForFrame
        0, // didFirstLayoutForFrame
        0, // didFirstVisuallyNonEmptyLayoutForFrame
        0, // didRemoveFrameFromHierarchy
        0, // didDisplayInsecureContentForFrame
        0, // didRunInsecureContentForFrame
        didClearWindowForFrame,
        0, // didCancelClientRedirectForFrame
        0, // willPerformClientRedirectForFrame
        0, // didHandleOnloadEventsForFrame
        0, // didLayoutForFrame
        0, // didNewFirstVisuallyNonEmptyLayoutForFrame
        0, // didDetectXSSForFrame
        0, // shouldGoToBackForwardListItem
        0, // didCreateGlobalObjectForFrame
        0, // willDisconnectDOMWindowExtensionFromGlobalObject
        0, // didReconnectDOMWindowExtensionToGlobalObject
        0, // willDestroyGlobalObjectForDOMWindowExtension
        0, // didFinishProgress
        0, // shouldForceUniversalAccessFromLocalURL
        0, // didReceiveIntentForFrame
        0, // registerIntentServiceForFrame
        0, // didLayout
        0, // featuresUsedInPage
        0, // willLoadURLRequest
        0, // willLoadDataRequest
    };
    WKBundlePageSetPageLoaderClient(m_page, &loaderClient);
}

QtBuiltinBundlePage::~QtBuiltinBundlePage()
{
    if (!m_navigatorQtObject)
        return;
    WKBundleFrameRef frame = WKBundlePageGetMainFrame(m_page);
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContext(frame);
    JSValueUnprotect(context, m_navigatorQtObject);
}

void QtBuiltinBundlePage::didClearWindowForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void* clientInfo)
{
    static_cast<QtBuiltinBundlePage*>(const_cast<void*>(clientInfo))->didClearWindowForFrame(frame, world);
}

static JSValueRef qt_postMessageCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
{
    // FIXME: should it work regardless of the thisObject?

    if (argumentCount < 1 || !JSValueIsString(context, arguments[0]))
        return JSValueMakeUndefined(context);

    QtBuiltinBundlePage* bundlePage = reinterpret_cast<QtBuiltinBundlePage*>(JSObjectGetPrivate(thisObject));
    ASSERT(bundlePage);

    // FIXME: needed?
    if (!bundlePage->navigatorQtObjectEnabled())
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> jsContents = JSValueToStringCopy(context, arguments[0], 0);
    WKRetainPtr<WKStringRef> contents(AdoptWK, WKStringCreateWithJSString(jsContents.get()));
    bundlePage->postMessageFromNavigatorQtObject(contents.get());
    return JSValueMakeUndefined(context);
}

void QtBuiltinBundlePage::didClearWindowForFrame(WKBundleFrameRef frame, WKBundleScriptWorldRef world)
{
    if (!WKBundleFrameIsMainFrame(frame) || WKBundleScriptWorldNormalWorld() != world)
        return;
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, world);
    registerNavigatorQtObject(context);
}

void QtBuiltinBundlePage::postMessageFromNavigatorQtObject(WKStringRef contents)
{
    static WKStringRef messageName = WKStringCreateWithUTF8CString("MessageFromNavigatorQtObject");
    WKTypeRef body[] = { page(), contents };
    WKRetainPtr<WKArrayRef> messageBody(AdoptWK, WKArrayCreate(body, sizeof(body) / sizeof(WKTypeRef)));
    WKBundlePostMessage(m_bundle->toRef(), messageName, messageBody.get());
}

static JSObjectRef createWrappedMessage(JSGlobalContextRef context, WKStringRef data)
{
    static JSStringRef dataName = JSStringCreateWithUTF8CString("data");

    JSRetainPtr<JSStringRef> jsData = WKStringCopyJSString(data);
    JSObjectRef wrappedMessage = JSObjectMake(context, 0, 0);
    JSObjectSetProperty(context, wrappedMessage, dataName, JSValueMakeString(context, jsData.get()), kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, 0);
    return wrappedMessage;
}

void QtBuiltinBundlePage::didReceiveMessageToNavigatorQtObject(WKStringRef contents)
{
    static JSStringRef onmessageName = JSStringCreateWithUTF8CString("onmessage");

    if (!m_navigatorQtObject)
        return;

    WKBundleFrameRef frame = WKBundlePageGetMainFrame(m_page);
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContext(frame);

    JSValueRef onmessageValue = JSObjectGetProperty(context, m_navigatorQtObject, onmessageName, 0);
    if (!JSValueIsObject(context, onmessageValue))
        return;

    JSObjectRef onmessageFunction = JSValueToObject(context, onmessageValue, 0);
    if (!JSObjectIsFunction(context, onmessageFunction))
        return;

    JSObjectRef wrappedMessage = createWrappedMessage(context, contents);
    JSObjectCallAsFunction(context, onmessageFunction, 0, 1, &wrappedMessage, 0);
}

void QtBuiltinBundlePage::setNavigatorQtObjectEnabled(bool enabled)
{
    if (enabled == m_navigatorQtObjectEnabled)
        return;
    // Note that this will take effect only after the next page load.
    m_navigatorQtObjectEnabled = enabled;
}

void QtBuiltinBundlePage::registerNavigatorQtObject(JSGlobalContextRef context)
{
    static JSStringRef postMessageName = JSStringCreateWithUTF8CString("postMessage");
    static JSStringRef navigatorName = JSStringCreateWithUTF8CString("navigator");
    static JSStringRef qtName = JSStringCreateWithUTF8CString("qt");

    if (m_navigatorQtObject)
        JSValueUnprotect(context, m_navigatorQtObject);
    m_navigatorQtObject = JSObjectMake(context, navigatorQtObjectClass(), this);
    JSValueProtect(context, m_navigatorQtObject);

    JSObjectRef postMessage = JSObjectMakeFunctionWithCallback(context, postMessageName, qt_postMessageCallback);
    JSObjectSetProperty(context, m_navigatorQtObject, postMessageName, postMessage, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, 0);

    JSValueRef navigatorValue = JSObjectGetProperty(context, JSContextGetGlobalObject(context), navigatorName, 0);
    if (!JSValueIsObject(context, navigatorValue))
        return;
    JSObjectRef navigatorObject = JSValueToObject(context, navigatorValue, 0);
    JSObjectSetProperty(context, navigatorObject, qtName, m_navigatorQtObject, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, 0);
}

JSClassRef QtBuiltinBundlePage::navigatorQtObjectClass()
{
    static JSClassRef classRef = 0;
    if (!classRef) {
        const JSClassDefinition navigatorQtObjectClass = kJSClassDefinitionEmpty;
        classRef = JSClassCreate(&navigatorQtObjectClass);
    }
    return classRef;
}

} // namespace WebKit
