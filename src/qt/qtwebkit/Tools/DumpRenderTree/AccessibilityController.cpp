/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
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

#include "config.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityController.h"

#include "AccessibilityUIElement.h"
#include <JavaScriptCore/JSRetainPtr.h>

// Static Value Getters

static JSValueRef getFocusedElementCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, controller->focusedElement());
}

static JSValueRef getRootElementCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, controller->rootElement());
}

// Object Creation

void AccessibilityController::makeWindowObject(JSContextRef context, JSObjectRef windowObject, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> accessibilityControllerStr(Adopt, JSStringCreateWithUTF8CString("accessibilityController"));
    
    JSClassRef classRef = getJSClass();
    JSValueRef accessibilityControllerObject = JSObjectMake(context, classRef, this);
    JSClassRelease(classRef);

    JSObjectSetProperty(context, windowObject, accessibilityControllerStr.get(), accessibilityControllerObject, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, exception);
}

static JSValueRef logFocusEventsCallback(JSContextRef ctx, JSObjectRef, JSObjectRef thisObject, size_t, const JSValueRef[], JSValueRef*)
{
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    controller->setLogFocusEvents(true);
    return JSValueMakeUndefined(ctx);
}

static JSValueRef logValueChangeEventsCallback(JSContextRef ctx, JSObjectRef, JSObjectRef thisObject, size_t, const JSValueRef[], JSValueRef*)
{
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    controller->setLogValueChangeEvents(true);
    return JSValueMakeUndefined(ctx);
}

static JSValueRef logScrollingStartEventsCallback(JSContextRef ctx, JSObjectRef, JSObjectRef thisObject, size_t, const JSValueRef[], JSValueRef*)
{
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    controller->setLogScrollingStartEvents(true);
    return JSValueMakeUndefined(ctx);
}

static JSValueRef logAccessibilityEventsCallback(JSContextRef ctx, JSObjectRef, JSObjectRef thisObject, size_t, const JSValueRef[], JSValueRef*)
{
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    controller->setLogAccessibilityEvents(true);
    return JSValueMakeUndefined(ctx);
}

static JSValueRef getElementAtPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int x = 0;
    int y = 0;
    if (argumentCount == 2) {
        x = JSValueToNumber(context, arguments[0], exception);
        y = JSValueToNumber(context, arguments[1], exception);
    }
    
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    return AccessibilityUIElement::makeJSAccessibilityUIElement(context, controller->elementAtPoint(x, y));
}

static JSValueRef getAccessibleElementByIdCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSStringRef idAttribute = 0;
    if (argumentCount == 1)
        idAttribute = JSValueToStringCopy(context, arguments[0], exception);    
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    JSValueRef result = AccessibilityUIElement::makeJSAccessibilityUIElement(context, controller->accessibleElementById(idAttribute));
    if (idAttribute)
        JSStringRelease(idAttribute);
    return result;
}

static JSValueRef addNotificationListenerCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return JSValueMakeBoolean(context, false);
    
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    JSObjectRef callback = JSValueToObject(context, arguments[0], exception);
    bool succeeded = controller->addNotificationListener(callback);
    return JSValueMakeBoolean(context, succeeded);
}

static JSValueRef removeNotificationListenerCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    AccessibilityController* controller = static_cast<AccessibilityController*>(JSObjectGetPrivate(thisObject));
    controller->removeNotificationListener();
    return JSValueMakeUndefined(context);
}

JSClassRef AccessibilityController::getJSClass()
{
    static JSStaticFunction staticFunctions[] = {
        { "logFocusEvents", logFocusEventsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "logValueChangeEvents", logValueChangeEventsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "logScrollingStartEvents", logScrollingStartEventsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "logAccessibilityEvents", logAccessibilityEventsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "elementAtPoint", getElementAtPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "accessibleElementById", getAccessibleElementByIdCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addNotificationListener", addNotificationListenerCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "removeNotificationListener", removeNotificationListenerCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { 0, 0, 0 }
    };

    static JSStaticValue staticValues[] = {
        { "focusedElement", getFocusedElementCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "rootElement", getRootElementCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { 0, 0, 0, 0 }
    };

    static JSClassDefinition classDefinition = {
        0, kJSClassAttributeNone, "AccessibilityController", 0, staticValues, staticFunctions,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    return JSClassCreate(&classDefinition);
}

void AccessibilityController::resetToConsistentState()
{
    setLogFocusEvents(false);
    setLogValueChangeEvents(false);
    setLogScrollingStartEvents(false);
    setLogAccessibilityEvents(false);
}
#endif // HAVE(ACCESSIBILITY)
