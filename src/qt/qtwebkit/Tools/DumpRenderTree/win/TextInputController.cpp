/*
 * Copyright (C) 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Joone Hur <joone@kldp.org>
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

#include <JavaScriptCore/JSRetainPtr.h>
#include <wtf/RefPtr.h>

// Static Functions

static JSValueRef setMarkedTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> str(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    double from = JSValueToNumber(context, arguments[1], exception);
    ASSERT(!*exception);

    double length = JSValueToNumber(context, arguments[2], exception);
    ASSERT(!*exception);

    TextInputController* controller = static_cast<TextInputController*>(JSObjectGetPrivate(thisObject));
    
    if (controller)
        controller->setMarkedText(str.get(), from, length);

    return JSValueMakeUndefined(context);
}

static JSValueRef hasMarkedTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TextInputController* controller = static_cast<TextInputController*>(JSObjectGetPrivate(thisObject));
    
    if (controller)
        return JSValueMakeBoolean(context, controller->hasMarkedText());

    return JSValueMakeUndefined(context);
}

static JSValueRef unmarkTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TextInputController* controller = static_cast<TextInputController*>(JSObjectGetPrivate(thisObject));
    if (controller)
        controller->unmarkText();

    return JSValueMakeUndefined(context);
}

static JSValueRef markedRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TextInputController* controller = static_cast<TextInputController*>(JSObjectGetPrivate(thisObject));
    if (controller) {
        vector<int> range = controller->markedRange();
        if (range.size() == 2) {
            JSValueRef argumentsArrayValues[] = { JSValueMakeNumber(context, range[0]), JSValueMakeNumber(context, range[1]) };
            JSObjectRef result = JSObjectMakeArray(context, sizeof(argumentsArrayValues) / sizeof(JSValueRef), argumentsArrayValues, exception);
            ASSERT(!*exception);
            return result;
        }
    }

    return JSValueMakeUndefined(context);
}

static JSValueRef insertTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> str(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TextInputController* controller = static_cast<TextInputController*>(JSObjectGetPrivate(thisObject));

    if (controller)
        controller->insertText(str.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef firstRectForCharacterRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    double start = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);

    double length = JSValueToNumber(context, arguments[1], exception);
    ASSERT(!*exception);

    TextInputController* controller = static_cast<TextInputController*>(JSObjectGetPrivate(thisObject));

    if (controller) {
        vector<int> rect = controller->firstRectForCharacterRange(start, length);
        if (rect.size() == 4) {
            JSValueRef argumentsArrayValues[] = 
            { 
                JSValueMakeNumber(context, rect[0]), 
                JSValueMakeNumber(context, rect[1]), 
                JSValueMakeNumber(context, rect[2]), 
                JSValueMakeNumber(context, rect[3]), 
            };
            JSObjectRef result = JSObjectMakeArray(context, sizeof(argumentsArrayValues) / sizeof(JSValueRef), argumentsArrayValues, exception);
            ASSERT(!*exception);
            return result;
        }
    }

    return JSValueMakeUndefined(context);
}

static JSValueRef selectedRangeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TextInputController* controller = static_cast<TextInputController*>(JSObjectGetPrivate(thisObject));

    if (controller) {
        vector<int> rect = controller->selectedRange();
        if (rect.size() == 2) {
            JSValueRef argumentsArrayValues[] = { 
                JSValueMakeNumber(context, rect[0]), 
                JSValueMakeNumber(context, rect[1]), 
            };
            JSObjectRef result = JSObjectMakeArray(context, sizeof(argumentsArrayValues) / sizeof(JSValueRef), argumentsArrayValues, exception);
            ASSERT(!*exception);
            return result;
        }
    }

    return JSValueMakeUndefined(context);
}

// Object Creation

void TextInputController::makeWindowObject(JSContextRef context, JSObjectRef windowObject, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef>  textInputContollerStr(Adopt, JSStringCreateWithUTF8CString("textInputController"));

    JSClassRef classRef = getJSClass();
    JSValueRef textInputContollerObject = JSObjectMake(context, classRef, this);
    JSClassRelease(classRef);

    JSObjectSetProperty(context, windowObject, textInputContollerStr.get(), textInputContollerObject, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, exception);
}

JSClassRef TextInputController::getJSClass()
{
    static JSStaticValue* staticValues = TextInputController::staticValues();
    static JSStaticFunction* staticFunctions = TextInputController::staticFunctions();
    static JSClassDefinition classDefinition = 
    {
        0, kJSClassAttributeNone, "TextInputController", 0, staticValues, staticFunctions,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    return JSClassCreate(&classDefinition);
}

JSStaticValue* TextInputController::staticValues()
{
    static JSStaticValue staticValues[] = 
    {
        { 0, 0, 0, 0 }
    };
    return staticValues;
}

JSStaticFunction* TextInputController::staticFunctions()
{
    static JSStaticFunction staticFunctions[] = {
        { "setMarkedText", setMarkedTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "hasMarkedText", hasMarkedTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "unmarkText", unmarkTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "markedRange", markedRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "insertText", insertTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "firstRectForCharacterRange", firstRectForCharacterRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "selectedRange", selectedRangeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { 0, 0, 0 }
    };

    return staticFunctions;
}
