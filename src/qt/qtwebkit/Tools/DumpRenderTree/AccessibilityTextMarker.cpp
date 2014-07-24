/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
#include "AccessibilityTextMarker.h"

#include "AccessibilityUIElement.h"
#include <JavaScriptCore/JSRetainPtr.h>

// MARK: AccessibilityTextMarker

// Callback methods

AccessibilityTextMarker* toTextMarker(JSObjectRef object)
{
    return static_cast<AccessibilityTextMarker*>(JSObjectGetPrivate(object));
}

static JSValueRef isMarkerEqualCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return JSValueMakeBoolean(context, false);

    JSObjectRef otherMarker = JSValueToObject(context, arguments[0], exception);
    return JSValueMakeBoolean(context, toTextMarker(thisObject)->isEqual(toTextMarker(otherMarker)));
}

// Destruction

static void markerFinalize(JSObjectRef thisObject)
{
    delete toTextMarker(thisObject);
}

// Object Creation

JSObjectRef AccessibilityTextMarker::makeJSAccessibilityTextMarker(JSContextRef context, const AccessibilityTextMarker& element)
{
    return JSObjectMake(context, AccessibilityTextMarker::getJSClass(), new AccessibilityTextMarker(element));
}

JSClassRef AccessibilityTextMarker::getJSClass()
{
    static JSStaticValue staticValues[] = {
        { 0, 0, 0, 0 }
    };
    
    static JSStaticFunction staticFunctions[] = {
        { "isEqual", isMarkerEqualCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { 0, 0, 0 }
    };
    
    static JSClassDefinition classDefinition = {
        0, kJSClassAttributeNone, "AccessibilityTextMarker", 0, staticValues, staticFunctions,
        0, markerFinalize, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    static JSClassRef accessibilityTextMarkerClass = JSClassCreate(&classDefinition);
    return accessibilityTextMarkerClass;
}

// MARK: AccessibilityTextMarkerRange

// Callback methods

AccessibilityTextMarkerRange* toTextMarkerRange(JSObjectRef object)
{
    return static_cast<AccessibilityTextMarkerRange*>(JSObjectGetPrivate(object));
}

static JSValueRef isMarkerRangeEqualCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return JSValueMakeBoolean(context, false);
    
    JSObjectRef otherMarker = JSValueToObject(context, arguments[0], exception);
    return JSValueMakeBoolean(context, toTextMarkerRange(thisObject)->isEqual(toTextMarkerRange(otherMarker)));
}

// Destruction

static void markerRangeFinalize(JSObjectRef thisObject)
{
    delete toTextMarkerRange(thisObject);
}

// Object Creation

JSObjectRef AccessibilityTextMarkerRange::makeJSAccessibilityTextMarkerRange(JSContextRef context, const AccessibilityTextMarkerRange& element)
{
    return JSObjectMake(context, AccessibilityTextMarkerRange::getJSClass(), new AccessibilityTextMarkerRange(element));
}

JSClassRef AccessibilityTextMarkerRange::getJSClass()
{
    static JSStaticValue staticValues[] = {
        { 0, 0, 0, 0 }
    };
    
    static JSStaticFunction staticFunctions[] = {
        { "isEqual", isMarkerRangeEqualCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { 0, 0, 0 }
    };
    
    static JSClassDefinition classDefinition = {
        0, kJSClassAttributeNone, "AccessibilityTextMarkerRange", 0, staticValues, staticFunctions,
        0, markerRangeFinalize, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    static JSClassRef accessibilityTextMarkerRangeClass = JSClassCreate(&classDefinition);
    return accessibilityTextMarkerRangeClass;
}
