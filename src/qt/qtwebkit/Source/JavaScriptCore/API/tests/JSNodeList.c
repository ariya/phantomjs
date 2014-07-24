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
#include "JSValueRef.h"
#include <wtf/Assertions.h>

static JSValueRef JSNodeList_item(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(object);

    if (argumentCount > 0) {
        NodeList* nodeList = JSObjectGetPrivate(thisObject);
        ASSERT(nodeList);
        Node* node = NodeList_item(nodeList, (unsigned)JSValueToNumber(context, arguments[0], exception));
        if (node)
            return JSNode_new(context, node);
    }
    
    return JSValueMakeUndefined(context);
}

static JSStaticFunction JSNodeList_staticFunctions[] = {
    { "item", JSNodeList_item, kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSValueRef JSNodeList_length(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);
    
    NodeList* nodeList = JSObjectGetPrivate(thisObject);
    ASSERT(nodeList);
    return JSValueMakeNumber(context, NodeList_length(nodeList));
}

static JSStaticValue JSNodeList_staticValues[] = {
    { "length", JSNodeList_length, NULL, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0, 0 }
};

static JSValueRef JSNodeList_getProperty(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    NodeList* nodeList = JSObjectGetPrivate(thisObject);
    ASSERT(nodeList);
    double index = JSValueToNumber(context, JSValueMakeString(context, propertyName), exception);
    unsigned uindex = (unsigned)index;
    if (uindex == index) { /* false for NaN */
        Node* node = NodeList_item(nodeList, uindex);
        if (node)
            return JSNode_new(context, node);
    }
    
    return NULL;
}

static void JSNodeList_initialize(JSContextRef context, JSObjectRef thisObject)
{
    UNUSED_PARAM(context);

    NodeList* nodeList = JSObjectGetPrivate(thisObject);
    ASSERT(nodeList);
    
    NodeList_ref(nodeList);
}

static void JSNodeList_finalize(JSObjectRef thisObject)
{
    NodeList* nodeList = JSObjectGetPrivate(thisObject);
    ASSERT(nodeList);

    NodeList_deref(nodeList);
}

static JSClassRef JSNodeList_class(JSContextRef context)
{
    UNUSED_PARAM(context);

    static JSClassRef jsClass;
    if (!jsClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.staticValues = JSNodeList_staticValues;
        definition.staticFunctions = JSNodeList_staticFunctions;
        definition.getProperty = JSNodeList_getProperty;
        definition.initialize = JSNodeList_initialize;
        definition.finalize = JSNodeList_finalize;

        jsClass = JSClassCreate(&definition);
    }
    
    return jsClass;
}

JSObjectRef JSNodeList_new(JSContextRef context, NodeList* nodeList)
{
    return JSObjectMake(context, JSNodeList_class(context), nodeList);
}
