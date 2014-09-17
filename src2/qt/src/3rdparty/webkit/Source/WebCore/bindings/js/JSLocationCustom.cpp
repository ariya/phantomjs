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
#include "JSLocationCustom.h"

#include "Location.h"
#include <runtime/JSFunction.h>

using namespace JSC;

namespace WebCore {

static JSValue nonCachingStaticReplaceFunctionGetter(ExecState* exec, JSValue, const Identifier& propertyName)
{
    return new (exec) JSFunction(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->functionStructure(), 1, propertyName, jsLocationPrototypeFunctionReplace);
}

static JSValue nonCachingStaticReloadFunctionGetter(ExecState* exec, JSValue, const Identifier& propertyName)
{
    return new (exec) JSFunction(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->functionStructure(), 0, propertyName, jsLocationPrototypeFunctionReload);
}

static JSValue nonCachingStaticAssignFunctionGetter(ExecState* exec, JSValue, const Identifier& propertyName)
{
    return new (exec) JSFunction(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->functionStructure(), 1, propertyName, jsLocationPrototypeFunctionAssign);
}

bool JSLocation::getOwnPropertySlotDelegate(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
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
    if (allowsAccessFromFrame(exec, frame, message))
        return false;

    // Check for the few functions that we allow, even when called cross-domain.
    const HashEntry* entry = JSLocationPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry && (entry->attributes() & Function)) {
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

bool JSLocation::getOwnPropertyDescriptorDelegate(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    Frame* frame = impl()->frame();
    if (!frame) {
        descriptor.setUndefined();
        return true;
    }
    
    // throw out all cross domain access
    if (!allowsAccessFromFrame(exec, frame))
        return true;
    
    // Check for the few functions that we allow, even when called cross-domain.
    const HashEntry* entry = JSLocationPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    PropertySlot slot;
    if (entry && (entry->attributes() & Function)) {
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

bool JSLocation::putDelegate(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    Frame* frame = impl()->frame();
    if (!frame)
        return true;

    if (propertyName == exec->propertyNames().toString || propertyName == exec->propertyNames().valueOf)
        return true;

    bool sameDomainAccess = allowsAccessFromFrame(exec, frame);

    const HashEntry* entry = JSLocation::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (!entry) {
        if (sameDomainAccess)
            JSObject::put(exec, propertyName, value, slot);
        return true;
    }

    // Cross-domain access to the location is allowed when assigning the whole location,
    // but not when assigning the individual pieces, since that might inadvertently
    // disclose other parts of the original location.
    if (entry->propertyPutter() != setJSLocationHref && !sameDomainAccess)
        return true;

    return false;
}

bool JSLocation::deleteProperty(ExecState* exec, const Identifier& propertyName)
{
    // Only allow deleting by frames in the same origin.
    if (!allowsAccessFromFrame(exec, impl()->frame()))
        return false;
    return Base::deleteProperty(exec, propertyName);
}

void JSLocation::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    // Only allow the location object to enumerated by frames in the same origin.
    if (!allowsAccessFromFrame(exec, impl()->frame()))
        return;
    Base::getOwnPropertyNames(exec, propertyNames, mode);
}

void JSLocation::defineGetter(ExecState* exec, const Identifier& propertyName, JSObject* getterFunction, unsigned attributes)
{
    if (propertyName == exec->propertyNames().toString || propertyName == exec->propertyNames().valueOf)
        return;
    Base::defineGetter(exec, propertyName, getterFunction, attributes);
}

void JSLocation::setHref(ExecState* exec, JSValue value)
{
    UString href = value.toString(exec);
    if (exec->hadException())
        return;
    impl()->setHref(ustringToString(href), activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setProtocol(ExecState* exec, JSValue value)
{
    UString protocol = value.toString(exec);
    if (exec->hadException())
        return;
    ExceptionCode ec = 0;
    impl()->setProtocol(ustringToString(protocol), activeDOMWindow(exec), firstDOMWindow(exec), ec);
    setDOMException(exec, ec);
}

void JSLocation::setHost(ExecState* exec, JSValue value)
{
    UString host = value.toString(exec);
    if (exec->hadException())
        return;
    impl()->setHost(ustringToString(host), activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setHostname(ExecState* exec, JSValue value)
{
    UString hostname = value.toString(exec);
    if (exec->hadException())
        return;
    impl()->setHostname(ustringToString(hostname), activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setPort(ExecState* exec, JSValue value)
{
    UString port = value.toString(exec);
    if (exec->hadException())
        return;
    impl()->setPort(ustringToString(port), activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setPathname(ExecState* exec, JSValue value)
{
    UString pathname = value.toString(exec);
    if (exec->hadException())
        return;
    impl()->setPathname(ustringToString(pathname), activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setSearch(ExecState* exec, JSValue value)
{
    UString pathname = value.toString(exec);
    if (exec->hadException())
        return;
    impl()->setSearch(ustringToString(pathname), activeDOMWindow(exec), firstDOMWindow(exec));
}

void JSLocation::setHash(ExecState* exec, JSValue value)
{
    UString hash = value.toString(exec);
    if (exec->hadException())
        return;
    impl()->setHash(ustringToString(hash), activeDOMWindow(exec), firstDOMWindow(exec));
}

JSValue JSLocation::replace(ExecState* exec)
{
    UString urlString = exec->argument(0).toString(exec);
    if (exec->hadException())
        return jsUndefined();
    impl()->replace(ustringToString(urlString), activeDOMWindow(exec), firstDOMWindow(exec));
    return jsUndefined();
}

JSValue JSLocation::reload(ExecState* exec)
{
    impl()->reload(activeDOMWindow(exec));
    return jsUndefined();
}

JSValue JSLocation::assign(ExecState* exec)
{
    UString urlString = exec->argument(0).toString(exec);
    if (exec->hadException())
        return jsUndefined();
    impl()->assign(ustringToString(urlString), activeDOMWindow(exec), firstDOMWindow(exec));
    return jsUndefined();
}

JSValue JSLocation::toStringFunction(ExecState* exec)
{
    Frame* frame = impl()->frame();
    if (!frame || !allowsAccessFromFrame(exec, frame))
        return jsUndefined();

    return jsString(exec, impl()->toString());
}

bool JSLocationPrototype::putDelegate(ExecState* exec, const Identifier& propertyName, JSValue, PutPropertySlot&)
{
    return (propertyName == exec->propertyNames().toString || propertyName == exec->propertyNames().valueOf);
}

void JSLocationPrototype::defineGetter(ExecState* exec, const Identifier& propertyName, JSObject* getterFunction, unsigned attributes)
{
    if (propertyName == exec->propertyNames().toString || propertyName == exec->propertyNames().valueOf)
        return;
    Base::defineGetter(exec, propertyName, getterFunction, attributes);
}

} // namespace WebCore
