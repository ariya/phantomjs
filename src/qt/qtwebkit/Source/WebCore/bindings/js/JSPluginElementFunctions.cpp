/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSPluginElementFunctions.h"

#include "BridgeJSC.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "JSHTMLElement.h"
#include "PluginViewBase.h"

using namespace JSC;

namespace WebCore {

using namespace Bindings;
using namespace HTMLNames;

// Runtime object support code for JSHTMLAppletElement, JSHTMLEmbedElement and JSHTMLObjectElement.

static inline bool isPluginElement(Node* node)
{
    return node->hasTagName(objectTag) || node->hasTagName(embedTag) || node->hasTagName(appletTag);
}

Instance* pluginInstance(Node* node)
{
    if (!node)
        return 0;
    if (!isPluginElement(node))
        return 0;

    HTMLPlugInElement* plugInElement = static_cast<HTMLPlugInElement*>(node);
    // The plugin element holds an owning reference, so we don't have to.
    Instance* instance = plugInElement->getInstance().get();
    if (!instance || !instance->rootObject())
        return 0;
    return instance;
}

static JSObject* pluginScriptObjectFromPluginViewBase(HTMLPlugInElement* pluginElement, JSGlobalObject* globalObject)
{
    Widget* pluginWidget = pluginElement->pluginWidget();
    if (!pluginWidget)
        return 0;
    
    if (!pluginWidget->isPluginViewBase())
        return 0;

    PluginViewBase* pluginViewBase = toPluginViewBase(pluginWidget);
    return pluginViewBase->scriptObject(globalObject);
}

static JSObject* pluginScriptObjectFromPluginViewBase(JSHTMLElement* jsHTMLElement)
{
    HTMLElement* element = jsHTMLElement->impl();
    if (!isPluginElement(element))
        return 0;

    HTMLPlugInElement* pluginElement = static_cast<HTMLPlugInElement*>(element);
    return pluginScriptObjectFromPluginViewBase(pluginElement, jsHTMLElement->globalObject());
}

JSObject* pluginScriptObject(ExecState* exec, JSHTMLElement* jsHTMLElement)
{
    HTMLElement* element = jsHTMLElement->impl();
    if (!isPluginElement(element))
        return 0;

    HTMLPlugInElement* pluginElement = static_cast<HTMLPlugInElement*>(element);

    // First, see if we can ask the plug-in view for its script object.
    if (JSObject* scriptObject = pluginScriptObjectFromPluginViewBase(pluginElement, jsHTMLElement->globalObject()))
        return scriptObject;

    // Otherwise, fall back to getting the object from the instance.

    // The plugin element holds an owning reference, so we don't have to.
    Instance* instance = pluginElement->getInstance().get();
    if (!instance || !instance->rootObject())
        return 0;

    return instance->createRuntimeObject(exec);
}
    
JSValue runtimeObjectPropertyGetter(ExecState* exec, JSValue slotBase, PropertyName propertyName)
{
    JSHTMLElement* element = jsCast<JSHTMLElement*>(asObject(slotBase));
    JSObject* scriptObject = pluginScriptObject(exec, element);
    if (!scriptObject)
        return jsUndefined();
    
    return scriptObject->get(exec, propertyName);
}

bool runtimeObjectCustomGetOwnPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot, JSHTMLElement* element)
{
    JSObject* scriptObject = pluginScriptObject(exec, element);
    if (!scriptObject)
        return false;

    if (!scriptObject->hasProperty(exec, propertyName))
        return false;
    slot.setCustom(element, runtimeObjectPropertyGetter);
    return true;
}

bool runtimeObjectCustomGetOwnPropertyDescriptor(ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, JSHTMLElement* element)
{
    JSObject* scriptObject = pluginScriptObject(exec, element);
    if (!scriptObject)
        return false;
    if (!scriptObject->hasProperty(exec, propertyName))
        return false;
    PropertySlot slot;
    slot.setCustom(element, runtimeObjectPropertyGetter);
    // While we don't know what the plugin allows, we do know that we prevent
    // enumeration or deletion of properties, so we mark plugin properties
    // as DontEnum | DontDelete
    descriptor.setDescriptor(slot.getValue(exec, propertyName), DontEnum | DontDelete);
    return true;
}

bool runtimeObjectCustomPut(ExecState* exec, PropertyName propertyName, JSValue value, JSHTMLElement* element, PutPropertySlot& slot)
{
    JSObject* scriptObject = pluginScriptObject(exec, element);
    if (!scriptObject)
        return 0;
    if (!scriptObject->hasProperty(exec, propertyName))
        return false;
    scriptObject->methodTable()->put(scriptObject, exec, propertyName, value, slot);
    return true;
}

static EncodedJSValue JSC_HOST_CALL callPlugin(ExecState* exec)
{
    JSHTMLElement* element = jsCast<JSHTMLElement*>(exec->callee());

    // Get the plug-in script object.
    JSObject* scriptObject = pluginScriptObject(exec, element);
    ASSERT(scriptObject);

    size_t argumentCount = exec->argumentCount();
    MarkedArgumentBuffer argumentList;
    for (size_t i = 0; i < argumentCount; i++)
        argumentList.append(exec->argument(i));

    CallData callData;
    CallType callType = getCallData(scriptObject, callData);
    ASSERT(callType == CallTypeHost);

    // Call the object.
    JSValue result = call(exec, scriptObject, callType, callData, exec->hostThisValue(), argumentList);
    return JSValue::encode(result);
}

CallType runtimeObjectGetCallData(JSHTMLElement* element, CallData& callData)
{
    // First, ask the plug-in view base for its runtime object.
    if (JSObject* scriptObject = pluginScriptObjectFromPluginViewBase(element)) {
        CallData scriptObjectCallData;
        
        if (scriptObject->methodTable()->getCallData(scriptObject, scriptObjectCallData) == CallTypeNone)
            return CallTypeNone;

        callData.native.function = callPlugin;
        return CallTypeHost;
    }
    
    Instance* instance = pluginInstance(element->impl());
    if (!instance || !instance->supportsInvokeDefaultMethod())
        return CallTypeNone;
    callData.native.function = callPlugin;
    return CallTypeHost;
}

} // namespace WebCore
