/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "NPRuntimeObjectMap.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "JSNPObject.h"
#include "NPJSObject.h"
#include "NPRuntimeUtilities.h"
#include "PluginView.h"
#include "WebProcess.h"
#include <JavaScriptCore/Completion.h>
#include <JavaScriptCore/Error.h>
#include <JavaScriptCore/JSLock.h>
#include <JavaScriptCore/SourceCode.h>
#include <JavaScriptCore/Strong.h>
#include <JavaScriptCore/StrongInlines.h>
#include <WebCore/DOMWrapperWorld.h>
#include <WebCore/Frame.h>
#include <WebCore/Page.h>
#include <WebCore/PageThrottler.h>
#include <WebCore/ScriptController.h>

using namespace JSC;
using namespace WebCore;

namespace WebKit {


NPRuntimeObjectMap::NPRuntimeObjectMap(PluginView* pluginView)
    : m_pluginView(pluginView)
    , m_finalizationTimer(RunLoop::main(), this, &NPRuntimeObjectMap::invalidateQueuedObjects)
{
}

NPRuntimeObjectMap::PluginProtector::PluginProtector(NPRuntimeObjectMap* npRuntimeObjectMap)
{
    // If we're already in the plug-in view destructor, we shouldn't try to keep it alive.
    if (!npRuntimeObjectMap->m_pluginView->isBeingDestroyed())
        m_pluginView = npRuntimeObjectMap->m_pluginView;
}

NPRuntimeObjectMap::PluginProtector::~PluginProtector()
{
}

NPObject* NPRuntimeObjectMap::getOrCreateNPObject(VM& vm, JSObject* jsObject)
{
    // If this is a JSNPObject, we can just get its underlying NPObject.
    if (jsObject->classInfo() == &JSNPObject::s_info) {
        JSNPObject* jsNPObject = static_cast<JSNPObject*>(jsObject);
        NPObject* npObject = jsNPObject->npObject();
        
        retainNPObject(npObject);
        return npObject;
    }
    
    // First, check if we already know about this object.
    if (NPJSObject* npJSObject = m_npJSObjects.get(jsObject)) {
        retainNPObject(npJSObject);
        return npJSObject;
    }

    NPJSObject* npJSObject = NPJSObject::create(vm, this, jsObject);
    m_npJSObjects.set(jsObject, npJSObject);

    return npJSObject;
}

void NPRuntimeObjectMap::npJSObjectDestroyed(NPJSObject* npJSObject)
{
    // Remove the object from the map.
    ASSERT(m_npJSObjects.contains(npJSObject->jsObject()));
    m_npJSObjects.remove(npJSObject->jsObject());
}

JSObject* NPRuntimeObjectMap::getOrCreateJSObject(JSGlobalObject* globalObject, NPObject* npObject)
{
    // If this is an NPJSObject, we can just get the JSObject that it's wrapping.
    if (NPJSObject::isNPJSObject(npObject))
        return NPJSObject::toNPJSObject(npObject)->jsObject();
    
    if (JSNPObject* jsNPObject = m_jsNPObjects.get(npObject))
        return jsNPObject;

    JSNPObject* jsNPObject = JSNPObject::create(globalObject, this, npObject);
    weakAdd(m_jsNPObjects, npObject, JSC::PassWeak<JSNPObject>(jsNPObject, this, npObject));
    return jsNPObject;
}

JSValue NPRuntimeObjectMap::convertNPVariantToJSValue(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject, const NPVariant& variant)
{
    switch (variant.type) {
    case NPVariantType_Void:
        return jsUndefined();

    case NPVariantType_Null:
        return jsNull();

    case NPVariantType_Bool:
        return jsBoolean(variant.value.boolValue);

    case NPVariantType_Int32:
        return jsNumber(variant.value.intValue);

    case NPVariantType_Double:
        return jsNumber(variant.value.doubleValue);

    case NPVariantType_String:
        return jsString(exec, String::fromUTF8WithLatin1Fallback(variant.value.stringValue.UTF8Characters, 
                                                                 variant.value.stringValue.UTF8Length));
    case NPVariantType_Object:
        return getOrCreateJSObject(globalObject, variant.value.objectValue);
    }

    ASSERT_NOT_REACHED();
    return jsUndefined();
}

void NPRuntimeObjectMap::convertJSValueToNPVariant(ExecState* exec, JSValue value, NPVariant& variant)
{
    JSLockHolder lock(exec);

    VOID_TO_NPVARIANT(variant);
    
    if (value.isNull()) {
        NULL_TO_NPVARIANT(variant);
        return;
    }

    if (value.isUndefined()) {
        VOID_TO_NPVARIANT(variant);
        return;
    }

    if (value.isBoolean()) {
        BOOLEAN_TO_NPVARIANT(value.toBoolean(exec), variant);
        return;
    }

    if (value.isNumber()) {
        DOUBLE_TO_NPVARIANT(value.toNumber(exec), variant);
        return;
    }

    if (value.isString()) {
        NPString npString = createNPString(value.toString(exec)->value(exec).utf8());
        STRINGN_TO_NPVARIANT(npString.UTF8Characters, npString.UTF8Length, variant);
        return;
    }

    if (value.isObject()) {
        NPObject* npObject = getOrCreateNPObject(exec->vm(), asObject(value));
        OBJECT_TO_NPVARIANT(npObject, variant);
        return;
    }

    ASSERT_NOT_REACHED();
}

bool NPRuntimeObjectMap::evaluate(NPObject* npObject, const String& scriptString, NPVariant* result)
{
    Strong<JSGlobalObject> globalObject(this->globalObject()->vm(), this->globalObject());
    if (!globalObject)
        return false;

    if (m_pluginView && !m_pluginView->isBeingDestroyed()) {
        if (Page* page = m_pluginView->frame()->page())
            page->pageThrottler()->reportInterestingEvent();
    }

    ExecState* exec = globalObject->globalExec();
    
    JSLockHolder lock(exec);
    JSValue thisValue = getOrCreateJSObject(globalObject.get(), npObject);

    JSValue resultValue = JSC::evaluate(exec, makeSource(scriptString), thisValue);

    convertJSValueToNPVariant(exec, resultValue, *result);
    return true;
}

void NPRuntimeObjectMap::invalidate()
{
    Vector<NPJSObject*> npJSObjects;
    copyValuesToVector(m_npJSObjects, npJSObjects);

    // Deallocate all the object wrappers so we won't leak any JavaScript objects.
    for (size_t i = 0; i < npJSObjects.size(); ++i)
        deallocateNPObject(npJSObjects[i]);
    
    // We shouldn't have any NPJSObjects left now.
    ASSERT(m_npJSObjects.isEmpty());

    Vector<NPObject*> objects;

    for (HashMap<NPObject*, JSC::Weak<JSNPObject> >::iterator ptr = m_jsNPObjects.begin(), end = m_jsNPObjects.end(); ptr != end; ++ptr) {
        JSNPObject* jsNPObject = ptr->value.get();
        if (!jsNPObject) // Skip zombies.
            continue;
        objects.append(jsNPObject->leakNPObject());
    }

    m_jsNPObjects.clear();

    for (size_t i = 0; i < objects.size(); ++i)
        releaseNPObject(objects[i]);
    
    // Deal with any objects that were scheduled for delayed destruction
    if (m_npObjectsToFinalize.isEmpty())
        return;
    ASSERT(m_finalizationTimer.isActive());
    m_finalizationTimer.stop();
    invalidateQueuedObjects();
}

JSGlobalObject* NPRuntimeObjectMap::globalObject() const
{
    Frame* frame = m_pluginView->frame();
    if (!frame)
        return 0;

    return frame->script()->globalObject(pluginWorld());
}

ExecState* NPRuntimeObjectMap::globalExec() const
{
    JSGlobalObject* globalObject = this->globalObject();
    if (!globalObject)
        return 0;
    
    return globalObject->globalExec();
}

static String& globalExceptionString()
{
    DEFINE_STATIC_LOCAL(String, exceptionString, ());
    return exceptionString;
}

void NPRuntimeObjectMap::setGlobalException(const String& exceptionString)
{
    globalExceptionString() = exceptionString;
}
    
void NPRuntimeObjectMap::moveGlobalExceptionToExecState(ExecState* exec)
{
    if (globalExceptionString().isNull())
        return;

    {
        JSLockHolder lock(exec);
        throwError(exec, createError(exec, globalExceptionString()));
    }
    
    globalExceptionString() = String();
}

void NPRuntimeObjectMap::invalidateQueuedObjects()
{
    ASSERT(m_npObjectsToFinalize.size());
    // We deliberately re-request m_npObjectsToFinalize.size() as custom dealloc
    // functions may execute JS and so get more objects added to the dealloc queue
    for (size_t i = 0; i < m_npObjectsToFinalize.size(); ++i)
        deallocateNPObject(m_npObjectsToFinalize[i]);
    m_npObjectsToFinalize.clear();
}

void NPRuntimeObjectMap::addToInvalidationQueue(NPObject* npObject)
{
    if (trySafeReleaseNPObject(npObject))
        return;
    if (m_npObjectsToFinalize.isEmpty())
        m_finalizationTimer.startOneShot(0);
    ASSERT(m_finalizationTimer.isActive());
    m_npObjectsToFinalize.append(npObject);
}

void NPRuntimeObjectMap::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    JSNPObject* object = static_cast<JSNPObject*>(handle.get().asCell());
    weakRemove(m_jsNPObjects, static_cast<NPObject*>(context), object);
    addToInvalidationQueue(object->leakNPObject());
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
