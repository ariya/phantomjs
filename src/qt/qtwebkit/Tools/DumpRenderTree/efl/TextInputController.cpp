/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2012 Samsung Electronics
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextInputController.h"

#include "DumpRenderTree.h"
#include "DumpRenderTreeChrome.h"
#include "WebCoreSupport/DumpRenderTreeSupportEfl.h"
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>

static JSValueRef setMarkedTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!browser->mainView() || argumentCount < 3)
        return JSValueMakeUndefined(context);

    JSStringRef string = JSValueToStringCopy(context, arguments[0], exception);

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(string);
    char* text = new char[bufferSize];
    JSStringGetUTF8CString(string, text, bufferSize);
    JSStringRelease(string);

    int start = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    int length = static_cast<int>(JSValueToNumber(context, arguments[2], exception));

    DumpRenderTreeSupportEfl::setComposition(browser->mainView(), text, start, length);

    delete[] text;
    return JSValueMakeUndefined(context);
}

static JSValueRef hasMarkedTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!browser->mainView())
        return JSValueMakeUndefined(context);

    return JSValueMakeBoolean(context, DumpRenderTreeSupportEfl::hasComposition(browser->mainView()));
}

static JSValueRef markedRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!browser->mainView())
        return JSValueMakeUndefined(context);

    int start, length;
    if (!DumpRenderTreeSupportEfl::compositionRange(browser->mainView(), &start, &length))
        return JSValueMakeUndefined(context);

    JSValueRef arrayValues[2];
    arrayValues[0] = JSValueMakeNumber(context, start);
    arrayValues[1] = JSValueMakeNumber(context, length);
    JSObjectRef arrayObject = JSObjectMakeArray(context, 2, arrayValues, exception);
    return arrayObject;
}

static JSValueRef insertTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!browser->mainView() || argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSStringRef string = JSValueToStringCopy(context, arguments[0], exception);

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(string);
    char* text = new char[bufferSize];
    JSStringGetUTF8CString(string, text, bufferSize);
    JSStringRelease(string);

    DumpRenderTreeSupportEfl::confirmComposition(browser->mainView(), text);

    delete[] text;
    return JSValueMakeUndefined(context);
}

static JSValueRef unmarkTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!browser->mainView())
        return JSValueMakeUndefined(context);

    DumpRenderTreeSupportEfl::confirmComposition(browser->mainView(), 0);
    return JSValueMakeUndefined(context);
}

static JSValueRef firstRectForCharacterRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!browser->mainView() || argumentCount < 2)
        return JSValueMakeUndefined(context);

    int location = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    int length = static_cast<int>(JSValueToNumber(context, arguments[1], exception));

    WebCore::IntRect rect = DumpRenderTreeSupportEfl::firstRectForCharacterRange(browser->mainView(), location, length);

    JSValueRef arrayValues[4];
    arrayValues[0] = JSValueMakeNumber(context, rect.x());
    arrayValues[1] = JSValueMakeNumber(context, rect.y());
    arrayValues[2] = JSValueMakeNumber(context, rect.width());
    arrayValues[3] = JSValueMakeNumber(context, rect.height());
    JSObjectRef arrayObject = JSObjectMakeArray(context, 4, arrayValues, exception);
    return arrayObject;
}

static JSValueRef selectedRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!browser->mainView())
        return JSValueMakeUndefined(context);

    int start, length;
    if (!DumpRenderTreeSupportEfl::selectedRange(browser->mainView(), &start, &length))
        return JSValueMakeUndefined(context);

    JSValueRef arrayValues[2];
    arrayValues[0] = JSValueMakeNumber(context, start);
    arrayValues[1] = JSValueMakeNumber(context, length);
    JSObjectRef arrayObject = JSObjectMakeArray(context, 2, arrayValues, exception);
    return arrayObject;
}

static JSStaticFunction staticFunctions[] = {
    { "setMarkedText", setMarkedTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "hasMarkedText", hasMarkedTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "markedRange", markedRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "insertText", insertTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "unmarkText", unmarkTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "firstRectForCharacterRange", firstRectForCharacterRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "selectedRange", selectedRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
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

