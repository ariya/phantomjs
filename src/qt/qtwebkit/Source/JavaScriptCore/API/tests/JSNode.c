/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "JSNode.h"
#include "JSNodeList.h"
#include "JSObjectRef.h"
#include "JSStringRef.h"
#include "JSValueRef.h"
#include "Node.h"
#include "NodeList.h"
#include <wtf/Assertions.h>

static JSValueRef JSNode_appendChild(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(function);

    /* Example of throwing a type error for invalid values */
    if (!JSValueIsObjectOfClass(context, thisObject, JSNode_class(context))) {
        JSStringRef message = JSStringCreateWithUTF8CString("TypeError: appendChild can only be called on nodes");
        *exception = JSValueMakeString(context, message);
        JSStringRelease(message);
    } else if (argumentCount < 1 || !JSValueIsObjectOfClass(context, arguments[0], JSNode_class(context))) {
        JSStringRef message = JSStringCreateWithUTF8CString("TypeError: first argument to appendChild must be a node");
        *exception = JSValueMakeString(context, message);
        JSStringRelease(message);
    } else {
        Node* node = JSObjectGetPrivate(thisObject);
        Node* child = JSObjectGetPrivate(JSValueToObject(context, arguments[0], NULL));

        Node_appendChild(node, child);
    }

    return JSValueMakeUndefined(context);
}

static JSValueRef JSNode_removeChild(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(function);

    /* Example of ignoring invalid values */
    if (argumentCount > 0) {
        if (JSValueIsObjectOfClass(context, thisObject, JSNode_class(context))) {
            if (JSValueIsObjectOfClass(context, arguments[0], JSNode_class(context))) {
                Node* node = JSObjectGetPrivate(thisObject);
                Node* child = JSObjectGetPrivate(JSValueToObject(context, arguments[0], exception));
                
                Node_removeChild(node, child);
            }
        }
    }
    
    return JSValueMakeUndefined(context);
}

static JSValueRef JSNode_replaceChild(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(function);
    
    if (argumentCount > 1) {
        if (JSValueIsObjectOfClass(context, thisObject, JSNode_class(context))) {
            if (JSValueIsObjectOfClass(context, arguments[0], JSNode_class(context))) {
                if (JSValueIsObjectOfClass(context, arguments[1], JSNode_class(context))) {
                    Node* node = JSObjectGetPrivate(thisObject);
                    Node* newChild = JSObjectGetPrivate(JSValueToObject(context, arguments[0], exception));
                    Node* oldChild = JSObjectGetPrivate(JSValueToObject(context, arguments[1], exception));
                    
                    Node_replaceChild(node, newChild, oldChild);
                }
            }
        }
    }
    
    return JSValueMakeUndefined(context);
}

static JSStaticFunction JSNode_staticFunctions[] = {
    { "appendChild", JSNode_appendChild, kJSPropertyAttributeDontDelete },
    { "removeChild", JSNode_removeChild, kJSPropertyAttributeDontDelete },
    { "replaceChild", JSNode_replaceChild, kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSValueRef JSNode_getNodeType(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    Node* node = JSObjectGetPrivate(object);
    if (node) {
        JSStringRef nodeType = JSStringCreateWithUTF8CString(node->nodeType);
        JSValueRef value = JSValueMakeString(context, nodeType);
        JSStringRelease(nodeType);
        return value;
    }
    
    return NULL;
}

static JSValueRef JSNode_getChildNodes(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    Node* node = JSObjectGetPrivate(thisObject);
    ASSERT(node);
    return JSNodeList_new(context, NodeList_new(node));
}

static JSValueRef JSNode_getFirstChild(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);
    
    return JSValueMakeUndefined(context);
}

static JSStaticValue JSNode_staticValues[] = {
    { "nodeType", JSNode_getNodeType, NULL, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly },
    { "childNodes", JSNode_getChildNodes, NULL, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly },
    { "firstChild", JSNode_getFirstChild, NULL, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly },
    { 0, 0, 0, 0 }
};

static void JSNode_initialize(JSContextRef context, JSObjectRef object)
{
    UNUSED_PARAM(context);

    Node* node = JSObjectGetPrivate(object);
    ASSERT(node);

    Node_ref(node);
}

static void JSNode_finalize(JSObjectRef object)
{
    Node* node = JSObjectGetPrivate(object);
    ASSERT(node);

    Node_deref(node);
}

JSClassRef JSNode_class(JSContextRef context)
{
    UNUSED_PARAM(context);

    static JSClassRef jsClass;
    if (!jsClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.staticValues = JSNode_staticValues;
        definition.staticFunctions = JSNode_staticFunctions;
        definition.initialize = JSNode_initialize;
        definition.finalize = JSNode_finalize;

        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

JSObjectRef JSNode_new(JSContextRef context, Node* node)
{
    return JSObjectMake(context, JSNode_class(context), node);
}

JSObjectRef JSNode_construct(JSContextRef context, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);

    return JSNode_new(context, Node_new());
}
