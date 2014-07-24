/*
 * Copyright (C) 2011 Igalia S.L.
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

#include "config.h"
#include "TextInputController.h"

#include "DumpRenderTree.h"
#include "WebCoreSupport/DumpRenderTreeSupportGtk.h"
#include <GOwnPtrGtk.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <cstring>
#include <webkit/webkit.h>

static JSValueRef setMarkedTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    JSStringRef string = JSValueToStringCopy(context, arguments[0], exception);
    ASSERT(!exception || !*exception);

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(string);
    GOwnPtr<gchar> stringBuffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(string, stringBuffer.get(), bufferSize);
    JSStringRelease(string);

    int start = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);

    int length = static_cast<int>(JSValueToNumber(context, arguments[2], exception));
    ASSERT(!exception || !*exception);

    DumpRenderTreeSupportGtk::setComposition(view, stringBuffer.get(), start, length);
    return JSValueMakeUndefined(context);
}

static JSValueRef hasMarkedTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);
    return JSValueMakeBoolean(context, DumpRenderTreeSupportGtk::hasComposition(view));
}

static JSValueRef markedRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    int start, length;
    if (!DumpRenderTreeSupportGtk::compositionRange(view, &start, &length))
        return JSValueMakeUndefined(context);

    JSValueRef arrayValues[2];
    arrayValues[0] = JSValueMakeNumber(context, start);
    arrayValues[1] = JSValueMakeNumber(context, length);
    JSObjectRef arrayObject = JSObjectMakeArray(context, 2, arrayValues, exception);
    ASSERT(!exception || !*exception);
    return arrayObject;
}

static JSValueRef insertTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSStringRef string = JSValueToStringCopy(context, arguments[0], exception);
    ASSERT(!exception || !*exception);

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(string);
    GOwnPtr<gchar> stringBuffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(string, stringBuffer.get(), bufferSize);
    JSStringRelease(string);

    DumpRenderTreeSupportGtk::confirmComposition(view, stringBuffer.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef unmarkTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    DumpRenderTreeSupportGtk::confirmComposition(view, 0);
    return JSValueMakeUndefined(context);
}

static JSValueRef firstRectForCharacterRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int location = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);

    int length = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);

    cairo_rectangle_int_t rect;
    if (!DumpRenderTreeSupportGtk::firstRectForCharacterRange(view, location, length, &rect))
        return JSValueMakeUndefined(context);

    JSValueRef arrayValues[4];
    arrayValues[0] = JSValueMakeNumber(context, rect.x);
    arrayValues[1] = JSValueMakeNumber(context, rect.y);
    arrayValues[2] = JSValueMakeNumber(context, rect.width);
    arrayValues[3] = JSValueMakeNumber(context, rect.height);
    JSObjectRef arrayObject = JSObjectMakeArray(context, 4, arrayValues, exception);
    ASSERT(!exception || !*exception);

    return arrayObject;
}

static JSValueRef selectedRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    int start, length;
    if (!DumpRenderTreeSupportGtk::selectedRange(view, &start, &length))
        return JSValueMakeUndefined(context);

    JSValueRef arrayValues[2];
    arrayValues[0] = JSValueMakeNumber(context, start);
    arrayValues[1] = JSValueMakeNumber(context, length);
    JSObjectRef arrayObject = JSObjectMakeArray(context, 2, arrayValues, exception);
    ASSERT(!exception || !*exception);

    return arrayObject;
}

static JSValueRef doCommandCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSStringRef string = JSValueToStringCopy(context, arguments[0], exception);
    ASSERT(!exception || !*exception);

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(string);
    GOwnPtr<gchar> stringBuffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(string, stringBuffer.get(), bufferSize);
    JSStringRelease(string);

    DumpRenderTreeSupportGtk::doCommand(view, stringBuffer.get());
    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticFunctions[] = {
    { "setMarkedText", setMarkedTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "hasMarkedText", hasMarkedTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "markedRange", markedRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "insertText", insertTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "unmarkText", unmarkTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "firstRectForCharacterRange", firstRectForCharacterRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "selectedRange", selectedRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "doCommand", doCommandCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSClassRef getClass(JSContextRef context)
{
    static JSClassRef textInputControllerClass = 0;

    if (!textInputControllerClass) {
        JSClassDefinition classDefinition = {
                0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        classDefinition.staticFunctions = staticFunctions;

        textInputControllerClass = JSClassCreate(&classDefinition);
    }

    return textInputControllerClass;
}

JSObjectRef makeTextInputController(JSContextRef context)
{
    return JSObjectMake(context, getClass(context), 0);
}
