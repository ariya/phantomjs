/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006 Jon Shier (jshier@iastate.edu)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reseved.
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "config.h"
#include "JSLocation.h"

#include "Location.h"
#include <runtime/JSFunction.h>

using namespace JSC;

namespace WebCore {

static JSValue nonCachingStaticReplaceFunctionGetter(ExecState* exec, JSValue, PropertyName propertyName)
{
    return JSFunction::create(exec, exec->lexicalGlobalObject(), 1, propertyName.publicName(), jsLocationPrototypeFunctionReplace);
}

static JSValue nonCachingStaticReloadFunctionGetter(ExecState* exec, JSValue, PropertyName propertyName)
{
    return JSFunction::create(exec, exec->lexicalGlobalObject(), 0, propertyName.publicName(), jsLocationPrototypeFunctionReload);
}

static JSValue nonCachingStaticAssignFunctionGetter(ExecState* exec, JSValue, PropertyName propertyName)
{
    return JSFunction::create(exec, exec->lexicalGlobalObject(), 1, propertyName.publicName(), jsLocationPrototypeFunctionAssign);
}

bool JSLocation::getOwnPropertySlotDelegate(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    Frame* frame = impl()->frame();
    if (!frame) {
        slot.setUndefined();
        return true;
    }

    // When accessing Location cross-domain, functions are always the native built-in ones.
    // See JSDOMWindow::getOwnPropertySlotDelegate for additional details.

    // Our custom code is only needed to implement the Window cross-domain scheme, so if access is
    // allowed, return false so the normal lookup will take place.
    String message;
    if (shouldAllowAccessToFrame(exec, frame, message))
        return false;

    // Check for the few functions that we allow, even when called cross-domain.
    const HashEntry* entry = JSLocationPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry && (entry->attributes() & JSC::Function)) {
        if (entry->function() == jsLocationPrototypeFunctionReplace) {
            slot.setCustom(this, nonCachingStaticReplaceFunctionGetter);
            return true;
        } else if (entry->function() == jsLocationPrototypeFunctionReload) {
            slot.setCustom(this, nonCachingStaticReloadFunctionGetter);
            return true;
        } else if (entry->function() == jsLocationPrototypeFunctionAssign) {
            slot.setCustom(this, nonCachingStaticAssignFunctionGetter);
            return true;
        }
    }

    // FIXME: Other implementers of the Window cross-domain scheme (Window, History) allow toString,
    // but for now we have decided not to, partly because it seems silly to return "[Object Location]" in
    // such cases when normally the string form of Location would be the URL.

    printErrorMessageForFrame(frame, message);
    slot.setUndefined();
    return true;
}

bool JSLocation::getOwnPropertyDescriptorDelegate(ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    Frame* frame = impl()->frame();
    if (!frame) {
        descriptor.setUndefined();
        return true;
    }
    
    // throw out all cross domain access
    if (!shouldAllowAccessToFrame(exec, frame))
        return true;
    
    // Check for the few functions that we allow, even when called cross-domain.
    const HashEntry* entry = JSLocationPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    PropertySlot slot;
    if (entry && (entry->attributes() & JSC::Function)) {
        if (entry->function() == jsLocationPrototypeFunctionReplace) {
            slot.setCustom(this, nonCachingStaticReplaceFunctionGetter);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
            return true;
        } else if (entry->function() == jsLocationPrototypeFunctionReload) {
            slot.setCustom(this, nonCachingStaticReloadFunctionGetter);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
            return true;
        } else if (entry->function() == jsLocationPrototypeFunctionAssign) {
            slot.setCustom(this, nonCachingStaticAssignFunctionGetter);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
            return true;
        }
    }
    
    // FIXME: Other implementers of the Window cross-domain scheme (Window, History) allow toString,
    // but for now we have decided not to, partly because it seems silly to return "[Object Location]" in
    // such cases when normally the string form of Location would be the URL.

    descriptor.setUndefined();
    return true;
}

bool JSLocation::putDelegate(ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    Frame* frame = impl()->frame();
    if (!frame)
        return true;

    if (propertyName == exec->propertyNames().toString || propertyName == exec->propertyNames().valueOf)
        return true;

    bool sameDomainAccess = shouldAllowAccessToFrame(exec, frame);

    const HashEntry* entry = JSLocation::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (!entry) {
        if (sameDomainAccess)
            JSObject::put(this, exec, propertyName, value, slot);
        return true;
    }

    // Cross-domain access to the location is allowed when assigning the whole location,
    // but not when assigning the individual pieces, since that might inadvertently
    // disclose other parts of the original location.
    if (entry->propertyPutter() != setJSLocationHref && !sameDomainAccess)
        return true;

    return false;
}

bool JSLocation::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSLocation* thisObject = jsCast<JSLocation*>(cell);
    // Only allow deleting by frames in the same origin.
    if (!shouldAllowAccessToFrame(exec, thisObject->impl()->frame()))
        return false;
    return Base::deleteProperty(thisObject, exec, propertyName);
}

bool JSLocation::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSLocation* thisObject = jsCast<JSLocation*>(cell);
    // Only allow deleting by frames in the same origin.
    if (!shouldAllowAccessToFrame(exec, thisObject->impl()->frame()))
        return false;
    return Base::deletePropertyByIndex(thisObject, exec, propertyName);
}

void JSLocation::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSLocation* thisObject = jsCast<JSLocation*>(object);
    // Only allow the location object to enumerated by frames in the same origin.
    if (!shouldAllowAccessToFrame(exec, thisObject->impl()->frame()))
        return;
    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

bool JSLocation::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, bool throwException)
{
    if (descriptor.isAccessorDescriptor() && (propertyName == exec->propertyNames().toString || propertyName == exec->propertyNames().valueOf))
        return false;
    return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);
}

void JSLocation::setHref(ExecState* exec, JSValue value)
{
    String href = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;
    impl()->setHref(href, activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setProtocol(ExecState* exec, JSValue value)
{
    String protocol = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;
    ExceptionCode ec = 0;
    impl()->setProtocol(protocol, activeDOMWindow(exec), firstDOMWindow(exec), ec);
    setDOMException(exec, ec);
}

void JSLocation::setHost(ExecState* exec, JSValue value)
{
    String host = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;
    impl()->setHost(host, activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setHostname(ExecState* exec, JSValue value)
{
    String hostname = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;
    impl()->setHostname(hostname, activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setPort(ExecState* exec, JSValue value)
{
    String port = value.toWTFString(exec);
    if (exec->hadException())
        return;
    impl()->setPort(port, activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setPathname(ExecState* exec, JSValue value)
{
    String pathname = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;
    impl()->setPathname(pathname, activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setSearch(ExecState* exec, JSValue value)
{
    String pathname = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;
    impl()->setSearch(pathname, activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setHash(ExecState* exec, JSValue value)
{
    String hash = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;
    impl()->setHash(hash, activeDOMWindow(exec), firstDOMWindow(exec));
}

JSValue JSLocation::replace(ExecState* exec)
{
    String urlString = exec->argument(0).toString(exec)->value(exec);
    if (exec->hadException())
        return jsUndefined();
    impl()->replace(urlString, activeDOMWindow(exec), firstDOMWindow(exec));
    return jsUndefined();
}

JSValue JSLocation::reload(ExecState* exec)
{
    impl()->reload(activeDOMWindow(exec));
    return jsUndefined();
}

JSValue JSLocation::assign(ExecState* exec)
{
    String urlString = exec->argument(0).toString(exec)->value(exec);
    if (exec->hadException())
        return jsUndefined();
    impl()->assign(urlString, activeDOMWindow(exec), firstDOMWindow(exec));
    return jsUndefined();
}

JSValue JSLocation::toStringFunction(ExecState* exec)
{
    Frame* frame = impl()->frame();
    if (!frame || !shouldAllowAccessToFrame(exec, frame))
        return jsUndefined();

    return jsStringWithCache(exec, impl()->toString());
}

bool JSLocationPrototype::putDelegate(ExecState* exec, PropertyName propertyName, JSValue, PutPropertySlot&)
{
    return (propertyName == exec->propertyNames().toString || propertyName == exec->propertyNames().valueOf);
}

bool JSLocationPrototype::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, bool throwException)
{
    if (descriptor.isAccessorDescriptor() && (propertyName == exec->propertyNames().toString || propertyName == exec->propertyNames().valueOf))
        return false;
    return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);
}

} // namespace WebCore
