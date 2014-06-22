/*
 *  Copyright (C) 1999 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef JSPluginElementFunctions_h
#define JSPluginElementFunctions_h

#include "JSDOMBinding.h"

namespace JSC {
namespace Bindings {
class Instance;
}
}

namespace WebCore {

    class HTMLElement;
    class JSHTMLElement;
    class Node;

    // Runtime object support code for JSHTMLAppletElement, JSHTMLEmbedElement and JSHTMLObjectElement.
    JSC::Bindings::Instance* pluginInstance(Node*);
    JSC::JSObject* pluginScriptObject(JSC::ExecState* exec, JSHTMLElement* jsHTMLElement);

    JSC::JSValue runtimeObjectPropertyGetter(JSC::ExecState*, JSC::JSValue, JSC::PropertyName);
    bool runtimeObjectCustomGetOwnPropertySlot(JSC::ExecState*, JSC::PropertyName, JSC::PropertySlot&, JSHTMLElement*);
    bool runtimeObjectCustomGetOwnPropertyDescriptor(JSC::ExecState*, JSC::PropertyName, JSC::PropertyDescriptor&, JSHTMLElement*);
    bool runtimeObjectCustomPut(JSC::ExecState*, JSC::PropertyName, JSC::JSValue, JSHTMLElement*, JSC::PutPropertySlot&);
    JSC::CallType runtimeObjectGetCallData(JSHTMLElement*, JSC::CallData&);

    template <class Type, class Base> bool pluginElementCustomGetOwnPropertySlot(JSC::ExecState* exec, JSC::PropertyName propertyName, JSC::PropertySlot& slot, Type* element)
    {
        if (!element->globalObject()->world()->isNormal()) {
            if (JSC::getStaticValueSlot<Type, Base>(exec, element->s_info.staticPropHashTable, element, propertyName, slot))
                return true;

            JSC::JSValue proto = element->prototype();
            if (proto.isObject() && JSC::jsCast<JSC::JSObject*>(asObject(proto))->hasProperty(exec, propertyName))
                return false;
        }
        
        return runtimeObjectCustomGetOwnPropertySlot(exec, propertyName, slot, element);
    }

    template <class Type, class Base> bool pluginElementCustomGetOwnPropertyDescriptor(JSC::ExecState* exec, JSC::PropertyName propertyName, JSC::PropertyDescriptor& descriptor, Type* element)
    {
        if (!element->globalObject()->world()->isNormal()) {
            if (JSC::getStaticValueDescriptor<Type, Base>(exec, element->s_info.staticPropHashTable, element, propertyName, descriptor))
                return true;

            JSC::JSValue proto = element->prototype();
            if (proto.isObject() && JSC::jsCast<JSC::JSObject*>(asObject(proto))->hasProperty(exec, propertyName))
                return false;
        }

        return runtimeObjectCustomGetOwnPropertyDescriptor(exec, propertyName, descriptor, element);
    }

} // namespace WebCore

#endif // JSPluginElementFunctions_h
