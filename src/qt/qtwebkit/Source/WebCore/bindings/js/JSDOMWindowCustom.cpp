/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "JSDOMWindowCustom.h"

#include "BindingSecurity.h"
#include "Frame.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "JSHTMLAudioElement.h"
#include "JSHTMLCollection.h"
#include "JSHTMLOptionElement.h"
#include "JSImageConstructor.h"
#include "JSMessagePortCustom.h"
#include "Location.h"
#include "ScheduledAction.h"
#include "Settings.h"
#include "SharedWorkerRepository.h"

#if ENABLE(WORKERS)
#include "JSWorker.h"
#endif

#if ENABLE(SHARED_WORKERS)
#include "JSSharedWorker.h"
#endif

#if ENABLE(WEB_AUDIO)
#include "JSAudioContext.h"
#endif

#if ENABLE(WEB_SOCKETS)
#include "JSWebSocket.h"
#endif

using namespace JSC;

namespace WebCore {

void JSDOMWindow::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);

    thisObject->impl()->visitJSEventListeners(visitor);
    if (Frame* frame = thisObject->impl()->frame())
        visitor.addOpaqueRoot(frame);
}

template<NativeFunction nativeFunction, int length>
JSValue nonCachingStaticFunctionGetter(ExecState* exec, JSValue, PropertyName propertyName)
{
    return JSFunction::create(exec, exec->lexicalGlobalObject(), length, propertyName.publicName(), nativeFunction);
}

static JSValue childFrameGetter(ExecState* exec, JSValue slotBase, PropertyName propertyName)
{
    return toJS(exec, jsCast<JSDOMWindow*>(asObject(slotBase))->impl()->frame()->tree()->scopedChild(propertyNameToAtomicString(propertyName))->document()->domWindow());
}

static JSValue indexGetter(ExecState* exec, JSValue slotBase, unsigned index)
{
    return toJS(exec, jsCast<JSDOMWindow*>(asObject(slotBase))->impl()->frame()->tree()->scopedChild(index)->document()->domWindow());
}

static JSValue namedItemGetter(ExecState* exec, JSValue slotBase, PropertyName propertyName)
{
    JSDOMWindowBase* thisObj = jsCast<JSDOMWindow*>(asObject(slotBase));
    Document* document = thisObj->impl()->frame()->document();

    ASSERT(BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObj->impl()));
    ASSERT(document);
    ASSERT(document->isHTMLDocument());

    AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
    if (!atomicPropertyName || !toHTMLDocument(document)->windowNamedItemMap().contains(atomicPropertyName))
        return jsUndefined();

    if (UNLIKELY(!toHTMLDocument(document)->windowNamedItemMap().containsSingle(atomicPropertyName))) {
        RefPtr<HTMLCollection> collection = document->windowNamedItems(atomicPropertyName);
        ASSERT(!collection->isEmpty());
        ASSERT(!collection->hasExactlyOneItem());
        return toJS(exec, thisObj->globalObject(), WTF::getPtr(collection));
    }

    Node* node = toHTMLDocument(document)->windowNamedItemMap().getElementByWindowNamedItem(atomicPropertyName, document);
    return toJS(exec, thisObj->globalObject(), node);
}

bool JSDOMWindow::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    // When accessing a Window cross-domain, functions are always the native built-in ones, and they
    // are not affected by properties changed on the Window or anything in its prototype chain.
    // This is consistent with the behavior of Firefox.

    const HashEntry* entry;

    // We don't want any properties other than "close" and "closed" on a frameless window (i.e. one whose page got closed,
    // or whose iframe got removed).
    // FIXME: This doesn't fully match Firefox, which allows at least toString in addition to those.
    if (!thisObject->impl()->frame()) {
        // The following code is safe for cross-domain and same domain use.
        // It ignores any custom properties that might be set on the DOMWindow (including a custom prototype).
        entry = s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && !(entry->attributes() & JSC::Function) && entry->propertyGetter() == jsDOMWindowClosed) {
            slot.setCustom(thisObject, entry->propertyGetter());
            return true;
        }
        entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && (entry->attributes() & JSC::Function) && entry->function() == jsDOMWindowPrototypeFunctionClose) {
            slot.setCustom(thisObject, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
            return true;
        }

        // FIXME: We should have a message here that explains why the property access/function call was
        // not allowed. 
        slot.setUndefined();
        return true;
    }

    // We need to check for cross-domain access here without printing the generic warning message
    // because we always allow access to some function, just different ones depending whether access
    // is allowed.
    String errorMessage;
    bool allowsAccess = shouldAllowAccessToDOMWindow(exec, thisObject->impl(), errorMessage);
    
    // Look for overrides before looking at any of our own properties, but ignore overrides completely
    // if this is cross-domain access.
    if (allowsAccess && JSGlobalObject::getOwnPropertySlot(thisObject, exec, propertyName, slot))
        return true;
    
    // We need this code here because otherwise JSDOMWindowBase will stop the search before we even get to the
    // prototype due to the blanket same origin (shouldAllowAccessToDOMWindow) check at the end of getOwnPropertySlot.
    // Also, it's important to get the implementation straight out of the DOMWindow prototype regardless of
    // what prototype is actually set on this object.
    entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        if (entry->attributes() & JSC::Function) {
            if (entry->function() == jsDOMWindowPrototypeFunctionBlur) {
                if (!allowsAccess) {
                    slot.setCustom(thisObject, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionBlur, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionClose) {
                if (!allowsAccess) {
                    slot.setCustom(thisObject, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionFocus) {
                if (!allowsAccess) {
                    slot.setCustom(thisObject, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionFocus, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionPostMessage) {
                if (!allowsAccess) {
                    slot.setCustom(thisObject, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionPostMessage, 2>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionShowModalDialog) {
                if (!DOMWindow::canShowModalDialog(thisObject->impl()->frame())) {
                    slot.setUndefined();
                    return true;
                }
            }
        }
    } else {
        // Allow access to toString() cross-domain, but always Object.prototype.toString.
        if (propertyName == exec->propertyNames().toString) {
            if (!allowsAccess) {
                slot.setCustom(thisObject, objectToStringFunctionGetter);
                return true;
            }
        }
    }

    entry = JSDOMWindow::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        slot.setCustom(thisObject, entry->propertyGetter());
        return true;
    }

    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (thisObject->impl()->frame()->tree()->scopedChild(propertyNameToAtomicString(propertyName))) {
        slot.setCustom(thisObject, childFrameGetter);
        return true;
    }

    // Do prototype lookup early so that functions and attributes in the prototype can have
    // precedence over the index and name getters.  
    JSValue proto = thisObject->prototype();
    if (proto.isObject()) {
        if (asObject(proto)->getPropertySlot(exec, propertyName, slot)) {
            if (!allowsAccess) {
                thisObject->printErrorMessage(errorMessage);
                slot.setUndefined();
            }
            return true;
        }
    }

    // FIXME: Search the whole frame hierarchy somewhere around here.
    // We need to test the correct priority order.

    // allow window[1] or parent[1] etc. (#56983)
    unsigned i = propertyName.asIndex();
    if (i < thisObject->impl()->frame()->tree()->scopedChildCount()) {
        ASSERT(i != PropertyName::NotAnIndex);
        slot.setCustomIndex(thisObject, i, indexGetter);
        return true;
    }

    if (!allowsAccess) {
        thisObject->printErrorMessage(errorMessage);
        slot.setUndefined();
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = thisObject->impl()->frame()->document();
    if (document->isHTMLDocument()) {
        AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
        if (atomicPropertyName && toHTMLDocument(document)->windowNamedItemMap().contains(atomicPropertyName)) {
            slot.setCustom(thisObject, namedItemGetter);
            return true;
        }
    }

    return Base::getOwnPropertySlot(thisObject, exec, propertyName, slot);
}

bool JSDOMWindow::getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned index, PropertySlot& slot)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    
    if (!thisObject->impl()->frame()) {
        // FIXME: We should have a message here that explains why the property access/function call was
        // not allowed. 
        slot.setUndefined();
        return true;
    }

    // We need to check for cross-domain access here without printing the generic warning message
    // because we always allow access to some function, just different ones depending whether access
    // is allowed.
    String errorMessage;
    bool allowsAccess = shouldAllowAccessToDOMWindow(exec, thisObject->impl(), errorMessage);

    // Look for overrides before looking at any of our own properties, but ignore overrides completely
    // if this is cross-domain access.
    if (allowsAccess && JSGlobalObject::getOwnPropertySlotByIndex(thisObject, exec, index, slot))
        return true;
    
    PropertyName propertyName = Identifier::from(exec, index);
    
    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (thisObject->impl()->frame()->tree()->scopedChild(propertyNameToAtomicString(propertyName))) {
        slot.setCustom(thisObject, childFrameGetter);
        return true;
    }
    
    // Do prototype lookup early so that functions and attributes in the prototype can have
    // precedence over the index and name getters.  
    JSValue proto = thisObject->prototype();
    if (proto.isObject()) {
        if (asObject(proto)->getPropertySlot(exec, index, slot)) {
            if (!allowsAccess) {
                thisObject->printErrorMessage(errorMessage);
                slot.setUndefined();
            }
            return true;
        }
    }

    // FIXME: Search the whole frame hierarchy somewhere around here.
    // We need to test the correct priority order.

    // allow window[1] or parent[1] etc. (#56983)
    if (index < thisObject->impl()->frame()->tree()->scopedChildCount()) {
        ASSERT(index != PropertyName::NotAnIndex);
        slot.setCustomIndex(thisObject, index, indexGetter);
        return true;
    }

    if (!allowsAccess) {
        thisObject->printErrorMessage(errorMessage);
        slot.setUndefined();
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = thisObject->impl()->frame()->document();
    if (document->isHTMLDocument()) {
        AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
        if (atomicPropertyName && toHTMLDocument(document)->windowNamedItemMap().contains(atomicPropertyName)) {
            slot.setCustom(thisObject, namedItemGetter);
            return true;
        }
    }

    return Base::getOwnPropertySlotByIndex(thisObject, exec, index, slot);
}

bool JSDOMWindow::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Never allow cross-domain getOwnPropertyDescriptor
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return false;

    const HashEntry* entry;
    
    // We don't want any properties other than "close" and "closed" on a closed window.
    if (!thisObject->impl()->frame()) {
        // The following code is safe for cross-domain and same domain use.
        // It ignores any custom properties that might be set on the DOMWindow (including a custom prototype).
        entry = s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && !(entry->attributes() & JSC::Function) && entry->propertyGetter() == jsDOMWindowClosed) {
            descriptor.setDescriptor(jsBoolean(true), ReadOnly | DontDelete | DontEnum);
            return true;
        }
        entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && (entry->attributes() & JSC::Function) && entry->function() == jsDOMWindowPrototypeFunctionClose) {
            PropertySlot slot;
            slot.setCustom(thisObject, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
            return true;
        }
        descriptor.setUndefined();
        return true;
    }

    entry = JSDOMWindow::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        PropertySlot slot;
        slot.setCustom(thisObject, entry->propertyGetter());
        descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
        return true;
    }
    
    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (thisObject->impl()->frame()->tree()->scopedChild(propertyNameToAtomicString(propertyName))) {
        PropertySlot slot;
        slot.setCustom(thisObject, childFrameGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
        return true;
    }
    
    unsigned i = propertyName.asIndex();
    if (i < thisObject->impl()->frame()->tree()->scopedChildCount()) {
        ASSERT(i != PropertyName::NotAnIndex);
        PropertySlot slot;
        slot.setCustomIndex(thisObject, i, indexGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = thisObject->impl()->frame()->document();
    if (document->isHTMLDocument()) {
        AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
        if (atomicPropertyName && toHTMLDocument(document)->windowNamedItemMap().contains(atomicPropertyName)) {
            PropertySlot slot;
            slot.setCustom(thisObject, namedItemGetter);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
            return true;
        }
    }
    
    return Base::getOwnPropertyDescriptor(thisObject, exec, propertyName, descriptor);
}

void JSDOMWindow::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    if (!thisObject->impl()->frame())
        return;

    // Optimization: access JavaScript global variables directly before involving the DOM.
    if (thisObject->JSGlobalObject::hasOwnPropertyForWrite(exec, propertyName)) {
        if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
            JSGlobalObject::put(thisObject, exec, propertyName, value, slot);
        return;
    }

    if (lookupPut<JSDOMWindow>(exec, propertyName, value, s_info.propHashTable(exec), thisObject))
        return;

    if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        Base::put(thisObject, exec, propertyName, value, slot);
}

void JSDOMWindow::putByIndex(JSCell* cell, ExecState* exec, unsigned index, JSValue value, bool shouldThrow)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    if (!thisObject->impl()->frame())
        return;
    
    PropertyName propertyName = Identifier::from(exec, index);

    // Optimization: access JavaScript global variables directly before involving the DOM.
    if (thisObject->JSGlobalObject::hasOwnPropertyForWrite(exec, propertyName)) {
        if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
            JSGlobalObject::putByIndex(thisObject, exec, index, value, shouldThrow);
        return;
    }
    
    if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        Base::putByIndex(thisObject, exec, index, value, shouldThrow);
}

bool JSDOMWindow::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    // Only allow deleting properties by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return false;
    return Base::deleteProperty(thisObject, exec, propertyName);
}

bool JSDOMWindow::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    // Only allow deleting properties by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return false;
    return Base::deletePropertyByIndex(thisObject, exec, propertyName);
}

void JSDOMWindow::getPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow the window to enumerated by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return;
    Base::getPropertyNames(thisObject, exec, propertyNames, mode);
}

void JSDOMWindow::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow the window to enumerated by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return;
    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

bool JSDOMWindow::defineOwnProperty(JSC::JSObject* object, JSC::ExecState* exec, JSC::PropertyName propertyName, JSC::PropertyDescriptor& descriptor, bool shouldThrow)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow defining properties in this way by frames in the same origin, as it allows setters to be introduced.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->impl()))
        return false;

    // Don't allow shadowing location using accessor properties.
    if (descriptor.isAccessorDescriptor() && propertyName == Identifier(exec, "location"))
        return false;

    return Base::defineOwnProperty(thisObject, exec, propertyName, descriptor, shouldThrow);
}

// Custom Attributes

void JSDOMWindow::setLocation(ExecState* exec, JSValue value)
{
#if ENABLE(DASHBOARD_SUPPORT)
    // To avoid breaking old widgets, make "var location =" in a top-level frame create
    // a property named "location" instead of performing a navigation (<rdar://problem/5688039>).
    if (Frame* activeFrame = activeDOMWindow(exec)->frame()) {
        if (Settings* settings = activeFrame->settings()) {
            if (settings->usesDashboardBackwardCompatibilityMode() && !activeFrame->tree()->parent()) {
                if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, impl()))
                    putDirect(exec->vm(), Identifier(exec, "location"), value);
                return;
            }
        }
    }
#endif

    String locationString = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;

    if (Location* location = impl()->location())
        location->setHref(locationString, activeDOMWindow(exec), firstDOMWindow(exec));
}

JSValue JSDOMWindow::event(ExecState* exec) const
{
    Event* event = currentEvent();
    if (!event)
        return jsUndefined();
    return toJS(exec, const_cast<JSDOMWindow*>(this), event);
}

JSValue JSDOMWindow::image(ExecState* exec) const
{
    return getDOMConstructor<JSImageConstructor>(exec, this);
}

// Custom functions

JSValue JSDOMWindow::open(ExecState* exec)
{
    String urlString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();
    AtomicString frameName = exec->argument(1).isUndefinedOrNull() ? "_blank" : exec->argument(1).toString(exec)->value(exec);
    if (exec->hadException())
        return jsUndefined();
    String windowFeaturesString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(2));
    if (exec->hadException())
        return jsUndefined();

    RefPtr<DOMWindow> openedWindow = impl()->open(urlString, frameName, windowFeaturesString, activeDOMWindow(exec), firstDOMWindow(exec));
    if (!openedWindow)
        return jsUndefined();
    return toJS(exec, openedWindow.get());
}

class DialogHandler {
public:
    explicit DialogHandler(ExecState* exec)
        : m_exec(exec)
    {
    }

    void dialogCreated(DOMWindow*);
    JSValue returnValue() const;

private:
    ExecState* m_exec;
    RefPtr<Frame> m_frame;
};

inline void DialogHandler::dialogCreated(DOMWindow* dialog)
{
    m_frame = dialog->frame();
    // FIXME: This looks like a leak between the normal world and an isolated
    //        world if dialogArguments comes from an isolated world.
    JSDOMWindow* globalObject = toJSDOMWindow(m_frame.get(), normalWorld(m_exec->vm()));
    if (JSValue dialogArguments = m_exec->argument(1))
        globalObject->putDirect(m_exec->vm(), Identifier(m_exec, "dialogArguments"), dialogArguments);
}

inline JSValue DialogHandler::returnValue() const
{
    JSDOMWindow* globalObject = toJSDOMWindow(m_frame.get(), normalWorld(m_exec->vm()));
    if (!globalObject)
        return jsUndefined();
    Identifier identifier(m_exec, "returnValue");
    PropertySlot slot;
    if (!JSGlobalObject::getOwnPropertySlot(globalObject, m_exec, identifier, slot))
        return jsUndefined();
    return slot.getValue(m_exec, identifier);
}

static void setUpDialog(DOMWindow* dialog, void* handler)
{
    static_cast<DialogHandler*>(handler)->dialogCreated(dialog);
}

JSValue JSDOMWindow::showModalDialog(ExecState* exec)
{
    String urlString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();
    String dialogFeaturesString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(2));
    if (exec->hadException())
        return jsUndefined();

    DialogHandler handler(exec);

    impl()->showModalDialog(urlString, dialogFeaturesString, activeDOMWindow(exec), firstDOMWindow(exec), setUpDialog, &handler);

    return handler.returnValue();
}

static JSValue handlePostMessage(DOMWindow* impl, ExecState* exec)
{
    MessagePortArray messagePorts;
    ArrayBufferArray arrayBuffers;

    // This function has variable arguments and can be:
    // Per current spec:
    //   postMessage(message, targetOrigin)
    //   postMessage(message, targetOrigin, {sequence of transferrables})
    // Legacy non-standard implementations in webkit allowed:
    //   postMessage(message, {sequence of transferrables}, targetOrigin);
    int targetOriginArgIndex = 1;
    if (exec->argumentCount() > 2) {
        int transferablesArgIndex = 2;
        if (exec->argument(2).isString()) {
            targetOriginArgIndex = 2;
            transferablesArgIndex = 1;
        }
        fillMessagePortArray(exec, exec->argument(transferablesArgIndex), messagePorts, arrayBuffers);
    }
    if (exec->hadException())
        return jsUndefined();

    RefPtr<SerializedScriptValue> message = SerializedScriptValue::create(exec, exec->argument(0),
                                                                         &messagePorts,
                                                                         &arrayBuffers);

    if (exec->hadException())
        return jsUndefined();

    String targetOrigin = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(targetOriginArgIndex));
    if (exec->hadException())
        return jsUndefined();

    ExceptionCode ec = 0;
    impl->postMessage(message.release(), &messagePorts, targetOrigin, activeDOMWindow(exec), ec);
    setDOMException(exec, ec);

    return jsUndefined();
}

JSValue JSDOMWindow::postMessage(ExecState* exec)
{
    return handlePostMessage(impl(), exec);
}

JSValue JSDOMWindow::setTimeout(ExecState* exec)
{
    ContentSecurityPolicy* contentSecurityPolicy = impl()->document() ? impl()->document()->contentSecurityPolicy() : 0;
    OwnPtr<ScheduledAction> action = ScheduledAction::create(exec, currentWorld(exec), contentSecurityPolicy);
    if (exec->hadException())
        return jsUndefined();

    if (!action)
        return jsNumber(0);

    int delay = exec->argument(1).toInt32(exec);

    ExceptionCode ec = 0;
    int result = impl()->setTimeout(action.release(), delay, ec);
    setDOMException(exec, ec);

    return jsNumber(result);
}

JSValue JSDOMWindow::setInterval(ExecState* exec)
{
    ContentSecurityPolicy* contentSecurityPolicy = impl()->document() ? impl()->document()->contentSecurityPolicy() : 0;
    OwnPtr<ScheduledAction> action = ScheduledAction::create(exec, currentWorld(exec), contentSecurityPolicy);
    if (exec->hadException())
        return jsUndefined();
    int delay = exec->argument(1).toInt32(exec);

    if (!action)
        return jsNumber(0);

    ExceptionCode ec = 0;
    int result = impl()->setInterval(action.release(), delay, ec);
    setDOMException(exec, ec);

    return jsNumber(result);
}

JSValue JSDOMWindow::addEventListener(ExecState* exec)
{
    Frame* frame = impl()->frame();
    if (!frame)
        return jsUndefined();

    JSValue listener = exec->argument(1);
    if (!listener.isObject())
        return jsUndefined();

    impl()->addEventListener(exec->argument(0).toString(exec)->value(exec), JSEventListener::create(asObject(listener), this, false, currentWorld(exec)), exec->argument(2).toBoolean(exec));
    return jsUndefined();
}

JSValue JSDOMWindow::removeEventListener(ExecState* exec)
{
    Frame* frame = impl()->frame();
    if (!frame)
        return jsUndefined();

    JSValue listener = exec->argument(1);
    if (!listener.isObject())
        return jsUndefined();

    impl()->removeEventListener(exec->argument(0).toString(exec)->value(exec), JSEventListener::create(asObject(listener), this, false, currentWorld(exec)).get(), exec->argument(2).toBoolean(exec));
    return jsUndefined();
}

DOMWindow* toDOMWindow(JSValue value)
{
    if (!value.isObject())
        return 0;
    JSObject* object = asObject(value);
    if (object->inherits(&JSDOMWindow::s_info))
        return jsCast<JSDOMWindow*>(object)->impl();
    if (object->inherits(&JSDOMWindowShell::s_info))
        return jsCast<JSDOMWindowShell*>(object)->impl();
    return 0;
}

} // namespace WebCore
