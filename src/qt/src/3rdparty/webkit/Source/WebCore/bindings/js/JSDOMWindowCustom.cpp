/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#include "Frame.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "History.h"
#include "JSArrayBuffer.h"
#include "JSAudioConstructor.h"
#include "JSDataView.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "JSEventSource.h"
#include "JSFloat32Array.h"
#include "JSHTMLCollection.h"
#include "JSHistory.h"
#include "JSImageConstructor.h"
#include "JSInt16Array.h"
#include "JSInt32Array.h"
#include "JSInt8Array.h"
#include "JSLocation.h"
#include "JSMessageChannel.h"
#include "JSMessagePortCustom.h"
#include "JSOptionConstructor.h"
#include "JSUint16Array.h"
#include "JSUint32Array.h"
#include "JSUint8Array.h"
#include "JSWebKitCSSMatrix.h"
#include "JSWebKitPoint.h"
#include "JSWorker.h"
#include "JSXMLHttpRequest.h"
#include "JSXSLTProcessor.h"
#include "Location.h"
#include "MediaPlayer.h"
#include "ScheduledAction.h"
#include "Settings.h"
#include "SharedWorkerRepository.h"
#include <runtime/JSFunction.h>

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

void JSDOMWindow::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);

    impl()->visitJSEventListeners(visitor);
    if (Frame* frame = impl()->frame())
        visitor.addOpaqueRoot(frame);
}

template<NativeFunction nativeFunction, int length>
JSValue nonCachingStaticFunctionGetter(ExecState* exec, JSValue, const Identifier& propertyName)
{
    return new (exec) JSFunction(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->functionStructure(), length, propertyName, nativeFunction);
}

static JSValue childFrameGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    return toJS(exec, static_cast<JSDOMWindow*>(asObject(slotBase))->impl()->frame()->tree()->child(identifierToAtomicString(propertyName))->domWindow());
}

static JSValue indexGetter(ExecState* exec, JSValue slotBase, unsigned index)
{
    return toJS(exec, static_cast<JSDOMWindow*>(asObject(slotBase))->impl()->frame()->tree()->child(index)->domWindow());
}

static JSValue namedItemGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    JSDOMWindowBase* thisObj = static_cast<JSDOMWindow*>(asObject(slotBase));
    Document* document = thisObj->impl()->frame()->document();

    ASSERT(thisObj->allowsAccessFrom(exec));
    ASSERT(document);
    ASSERT(document->isHTMLDocument());

    RefPtr<HTMLCollection> collection = document->windowNamedItems(identifierToString(propertyName));
    if (collection->length() == 1)
        return toJS(exec, thisObj, collection->firstItem());
    return toJS(exec, thisObj, collection.get());
}

bool JSDOMWindow::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // When accessing a Window cross-domain, functions are always the native built-in ones, and they
    // are not affected by properties changed on the Window or anything in its prototype chain.
    // This is consistent with the behavior of Firefox.

    const HashEntry* entry;

    // We don't want any properties other than "close" and "closed" on a closed window.
    if (!impl()->frame()) {
        // The following code is safe for cross-domain and same domain use.
        // It ignores any custom properties that might be set on the DOMWindow (including a custom prototype).
        entry = s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && !(entry->attributes() & Function) && entry->propertyGetter() == jsDOMWindowClosed) {
            slot.setCustom(this, entry->propertyGetter());
            return true;
        }
        entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && (entry->attributes() & Function) && entry->function() == jsDOMWindowPrototypeFunctionClose) {
            slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
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
    bool allowsAccess = allowsAccessFrom(exec, errorMessage);

    // Look for overrides before looking at any of our own properties, but ignore overrides completely
    // if this is cross-domain access.
    if (allowsAccess && JSGlobalObject::getOwnPropertySlot(exec, propertyName, slot))
        return true;

    // We need this code here because otherwise JSDOMWindowBase will stop the search before we even get to the
    // prototype due to the blanket same origin (allowsAccessFrom) check at the end of getOwnPropertySlot.
    // Also, it's important to get the implementation straight out of the DOMWindow prototype regardless of
    // what prototype is actually set on this object.
    entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        if (entry->attributes() & Function) {
            if (entry->function() == jsDOMWindowPrototypeFunctionBlur) {
                if (!allowsAccess) {
                    slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionBlur, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionClose) {
                if (!allowsAccess) {
                    slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionFocus) {
                if (!allowsAccess) {
                    slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionFocus, 0>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionPostMessage) {
                if (!allowsAccess) {
                    slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionPostMessage, 2>);
                    return true;
                }
            } else if (entry->function() == jsDOMWindowPrototypeFunctionShowModalDialog) {
                if (!DOMWindow::canShowModalDialog(impl()->frame())) {
                    slot.setUndefined();
                    return true;
                }
            }
        }
    } else {
        // Allow access to toString() cross-domain, but always Object.prototype.toString.
        if (propertyName == exec->propertyNames().toString) {
            if (!allowsAccess) {
                slot.setCustom(this, objectToStringFunctionGetter);
                return true;
            }
        }
    }

    entry = JSDOMWindow::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        slot.setCustom(this, entry->propertyGetter());
        return true;
    }

    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (impl()->frame()->tree()->child(identifierToAtomicString(propertyName))) {
        slot.setCustom(this, childFrameGetter);
        return true;
    }

    // Do prototype lookup early so that functions and attributes in the prototype can have
    // precedence over the index and name getters.  
    JSValue proto = prototype();
    if (proto.isObject()) {
        if (asObject(proto)->getPropertySlot(exec, propertyName, slot)) {
            if (!allowsAccess) {
                printErrorMessage(errorMessage);
                slot.setUndefined();
            }
            return true;
        }
    }

    // FIXME: Search the whole frame hierarchy somewhere around here.
    // We need to test the correct priority order.

    // allow window[1] or parent[1] etc. (#56983)
    bool ok;
    unsigned i = propertyName.toArrayIndex(ok);
    if (ok && i < impl()->frame()->tree()->childCount()) {
        slot.setCustomIndex(this, i, indexGetter);
        return true;
    }

    if (!allowsAccess) {
        printErrorMessage(errorMessage);
        slot.setUndefined();
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = impl()->frame()->document();
    if (document->isHTMLDocument()) {
        AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
        if (atomicPropertyName && (static_cast<HTMLDocument*>(document)->hasNamedItem(atomicPropertyName) || document->hasElementWithId(atomicPropertyName))) {
            slot.setCustom(this, namedItemGetter);
            return true;
        }
    }

    return Base::getOwnPropertySlot(exec, propertyName, slot);
}

bool JSDOMWindow::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    // Never allow cross-domain getOwnPropertyDescriptor
    if (!allowsAccessFrom(exec))
        return false;

    const HashEntry* entry;
    
    // We don't want any properties other than "close" and "closed" on a closed window.
    if (!impl()->frame()) {
        // The following code is safe for cross-domain and same domain use.
        // It ignores any custom properties that might be set on the DOMWindow (including a custom prototype).
        entry = s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && !(entry->attributes() & Function) && entry->propertyGetter() == jsDOMWindowClosed) {
            descriptor.setDescriptor(jsBoolean(true), ReadOnly | DontDelete | DontEnum);
            return true;
        }
        entry = JSDOMWindowPrototype::s_info.propHashTable(exec)->entry(exec, propertyName);
        if (entry && (entry->attributes() & Function) && entry->function() == jsDOMWindowPrototypeFunctionClose) {
            PropertySlot slot;
            slot.setCustom(this, nonCachingStaticFunctionGetter<jsDOMWindowPrototypeFunctionClose, 0>);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
            return true;
        }
        descriptor.setUndefined();
        return true;
    }

    entry = JSDOMWindow::s_info.propHashTable(exec)->entry(exec, propertyName);
    if (entry) {
        PropertySlot slot;
        slot.setCustom(this, entry->propertyGetter());
        descriptor.setDescriptor(slot.getValue(exec, propertyName), entry->attributes());
        return true;
    }
    
    // Check for child frames by name before built-in properties to
    // match Mozilla. This does not match IE, but some sites end up
    // naming frames things that conflict with window properties that
    // are in Moz but not IE. Since we have some of these, we have to do
    // it the Moz way.
    if (impl()->frame()->tree()->child(identifierToAtomicString(propertyName))) {
        PropertySlot slot;
        slot.setCustom(this, childFrameGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
        return true;
    }
    
    bool ok;
    unsigned i = propertyName.toArrayIndex(ok);
    if (ok && i < impl()->frame()->tree()->childCount()) {
        PropertySlot slot;
        slot.setCustomIndex(this, i, indexGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
        return true;
    }

    // Allow shortcuts like 'Image1' instead of document.images.Image1
    Document* document = impl()->frame()->document();
    if (document->isHTMLDocument()) {
        AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
        if (atomicPropertyName && (static_cast<HTMLDocument*>(document)->hasNamedItem(atomicPropertyName) || document->hasElementWithId(atomicPropertyName))) {
            PropertySlot slot;
            slot.setCustom(this, namedItemGetter);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
            return true;
        }
    }
    
    return Base::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

void JSDOMWindow::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    if (!impl()->frame())
        return;

    // Optimization: access JavaScript global variables directly before involving the DOM.
    if (JSGlobalObject::hasOwnPropertyForWrite(exec, propertyName)) {
        if (allowsAccessFrom(exec))
            JSGlobalObject::put(exec, propertyName, value, slot);
        return;
    }

    if (lookupPut<JSDOMWindow>(exec, propertyName, value, s_info.propHashTable(exec), this))
        return;

    if (allowsAccessFrom(exec))
        Base::put(exec, propertyName, value, slot);
}

bool JSDOMWindow::deleteProperty(ExecState* exec, const Identifier& propertyName)
{
    // Only allow deleting properties by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return false;
    return Base::deleteProperty(exec, propertyName);
}

void JSDOMWindow::getPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    // Only allow the window to enumerated by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return;
    Base::getPropertyNames(exec, propertyNames, mode);
}

void JSDOMWindow::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    // Only allow the window to enumerated by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return;
    Base::getOwnPropertyNames(exec, propertyNames, mode);
}

void JSDOMWindow::defineGetter(ExecState* exec, const Identifier& propertyName, JSObject* getterFunction, unsigned attributes)
{
    // Only allow defining getters by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return;

    // Don't allow shadowing location using defineGetter.
    if (propertyName == "location")
        return;

    Base::defineGetter(exec, propertyName, getterFunction, attributes);
}

void JSDOMWindow::defineSetter(ExecState* exec, const Identifier& propertyName, JSObject* setterFunction, unsigned attributes)
{
    // Only allow defining setters by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return;
    Base::defineSetter(exec, propertyName, setterFunction, attributes);
}

bool JSDOMWindow::defineOwnProperty(JSC::ExecState* exec, const JSC::Identifier& propertyName, JSC::PropertyDescriptor& descriptor, bool shouldThrow)
{
    // Only allow defining properties in this way by frames in the same origin, as it allows setters to be introduced.
    if (!allowsAccessFrom(exec))
        return false;
    return Base::defineOwnProperty(exec, propertyName, descriptor, shouldThrow);
}

JSValue JSDOMWindow::lookupGetter(ExecState* exec, const Identifier& propertyName)
{
    // Only allow looking-up getters by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return jsUndefined();
    return Base::lookupGetter(exec, propertyName);
}

JSValue JSDOMWindow::lookupSetter(ExecState* exec, const Identifier& propertyName)
{
    // Only allow looking-up setters by frames in the same origin.
    if (!allowsAccessFrom(exec))
        return jsUndefined();
    return Base::lookupSetter(exec, propertyName);
}

// Custom Attributes

JSValue JSDOMWindow::history(ExecState* exec) const
{
    History* history = impl()->history();
    if (JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), history))
        return wrapper;

    JSDOMWindow* window = const_cast<JSDOMWindow*>(this);
    JSHistory* jsHistory = new (exec) JSHistory(getDOMStructure<JSHistory>(exec, window), window, history);
    cacheWrapper(currentWorld(exec), history, jsHistory);
    return jsHistory;
}

JSValue JSDOMWindow::location(ExecState* exec) const
{
    Location* location = impl()->location();
    if (JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), location))
        return wrapper;

    JSDOMWindow* window = const_cast<JSDOMWindow*>(this);
    JSLocation* jsLocation = new (exec) JSLocation(getDOMStructure<JSLocation>(exec, window), window, location);
    cacheWrapper(currentWorld(exec), location, jsLocation);
    return jsLocation;
}

void JSDOMWindow::setLocation(ExecState* exec, JSValue value)
{
#if ENABLE(DASHBOARD_SUPPORT)
    // To avoid breaking old widgets, make "var location =" in a top-level frame create
    // a property named "location" instead of performing a navigation (<rdar://problem/5688039>).
    if (Frame* activeFrame = activeDOMWindow(exec)->frame()) {
        if (Settings* settings = activeFrame->settings()) {
            if (settings->usesDashboardBackwardCompatibilityMode() && !activeFrame->tree()->parent()) {
                if (allowsAccessFrom(exec))
                    putDirect(exec->globalData(), Identifier(exec, "location"), value);
                return;
            }
        }
    }
#endif

    UString locationString = value.toString(exec);
    if (exec->hadException())
        return;

    impl()->setLocation(ustringToString(locationString), activeDOMWindow(exec), firstDOMWindow(exec));
}

JSValue JSDOMWindow::event(ExecState* exec) const
{
    Event* event = currentEvent();
    if (!event)
        return jsUndefined();
    return toJS(exec, const_cast<JSDOMWindow*>(this), event);
}

#if ENABLE(EVENTSOURCE)
JSValue JSDOMWindow::eventSource(ExecState* exec) const
{
    return getDOMConstructor<JSEventSourceConstructor>(exec, this);
}
#endif

JSValue JSDOMWindow::image(ExecState* exec) const
{
    return getDOMConstructor<JSImageConstructor>(exec, this);
}

JSValue JSDOMWindow::option(ExecState* exec) const
{
    return getDOMConstructor<JSOptionConstructor>(exec, this);
}

#if ENABLE(VIDEO)
JSValue JSDOMWindow::audio(ExecState* exec) const
{
    if (!MediaPlayer::isAvailable())
        return jsUndefined();
    return getDOMConstructor<JSAudioConstructor>(exec, this);
}
#endif

JSValue JSDOMWindow::webKitPoint(ExecState* exec) const
{
    return getDOMConstructor<JSWebKitPointConstructor>(exec, this);
}

JSValue JSDOMWindow::webKitCSSMatrix(ExecState* exec) const
{
    return getDOMConstructor<JSWebKitCSSMatrixConstructor>(exec, this);
}
 
JSValue JSDOMWindow::arrayBuffer(ExecState* exec) const
{
    return getDOMConstructor<JSArrayBufferConstructor>(exec, this);
}
 
JSValue JSDOMWindow::int8Array(ExecState* exec) const
{
    return getDOMConstructor<JSInt8ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::uint8Array(ExecState* exec) const
{
    return getDOMConstructor<JSUint8ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::int32Array(ExecState* exec) const
{
    return getDOMConstructor<JSInt32ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::uint32Array(ExecState* exec) const
{
    return getDOMConstructor<JSUint32ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::int16Array(ExecState* exec) const
{
    return getDOMConstructor<JSInt16ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::uint16Array(ExecState* exec) const
{
    return getDOMConstructor<JSUint16ArrayConstructor>(exec, this);
}
 
JSValue JSDOMWindow::float32Array(ExecState* exec) const
{
    return getDOMConstructor<JSFloat32ArrayConstructor>(exec, this);
}

JSValue JSDOMWindow::dataView(ExecState* exec) const
{
    return getDOMConstructor<JSDataViewConstructor>(exec, this);
}

JSValue JSDOMWindow::xmlHttpRequest(ExecState* exec) const
{
    return getDOMConstructor<JSXMLHttpRequestConstructor>(exec, this);
}

#if ENABLE(XSLT)
JSValue JSDOMWindow::xsltProcessor(ExecState* exec) const
{
    return getDOMConstructor<JSXSLTProcessorConstructor>(exec, this);
}
#endif

#if ENABLE(CHANNEL_MESSAGING)
JSValue JSDOMWindow::messageChannel(ExecState* exec) const
{
    return getDOMConstructor<JSMessageChannelConstructor>(exec, this);
}
#endif

#if ENABLE(WORKERS)
JSValue JSDOMWindow::worker(ExecState* exec) const
{
    return getDOMConstructor<JSWorkerConstructor>(exec, this);
}
#endif

#if ENABLE(SHARED_WORKERS)
JSValue JSDOMWindow::sharedWorker(ExecState* exec) const
{
    if (SharedWorkerRepository::isAvailable())
        return getDOMConstructor<JSSharedWorkerConstructor>(exec, this);
    return jsUndefined();
}
#endif

#if ENABLE(WEB_AUDIO)
JSValue JSDOMWindow::webkitAudioContext(ExecState* exec) const
{
    return getDOMConstructor<JSAudioContextConstructor>(exec, this);
}
#endif

#if ENABLE(WEB_SOCKETS)
JSValue JSDOMWindow::webSocket(ExecState* exec) const
{
    Frame* frame = impl()->frame();
    if (!frame)
        return jsUndefined();
    Settings* settings = frame->settings();
    if (!settings)
        return jsUndefined();
    return getDOMConstructor<JSWebSocketConstructor>(exec, this);
}
#endif

// Custom functions

JSValue JSDOMWindow::open(ExecState* exec)
{
    String urlString = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    if (exec->hadException())
        return jsUndefined();
    AtomicString frameName = exec->argument(1).isUndefinedOrNull() ? "_blank" : ustringToAtomicString(exec->argument(1).toString(exec));
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
        , m_globalObject(0)
    {
    }

    void dialogCreated(DOMWindow*);
    JSValue returnValue() const;

private:
    ExecState* m_exec;
    JSDOMWindow* m_globalObject;
};

inline void DialogHandler::dialogCreated(DOMWindow* dialog)
{
    // FIXME: This looks like a leak between the normal world and an isolated
    //        world if dialogArguments comes from an isolated world.
    m_globalObject = toJSDOMWindow(dialog->frame(), normalWorld(m_exec->globalData()));
    if (JSValue dialogArguments = m_exec->argument(1))
        m_globalObject->putDirect(m_exec->globalData(), Identifier(m_exec, "dialogArguments"), dialogArguments);
}

inline JSValue DialogHandler::returnValue() const
{
    if (!m_globalObject)
        return jsUndefined();
    Identifier identifier(m_exec, "returnValue");
    PropertySlot slot;
    if (!m_globalObject->JSGlobalObject::getOwnPropertySlot(m_exec, identifier, slot))
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

JSValue JSDOMWindow::postMessage(ExecState* exec)
{
    PassRefPtr<SerializedScriptValue> message = SerializedScriptValue::create(exec, exec->argument(0));

    if (exec->hadException())
        return jsUndefined();

    MessagePortArray messagePorts;
    if (exec->argumentCount() > 2)
        fillMessagePortArray(exec, exec->argument(1), messagePorts);
    if (exec->hadException())
        return jsUndefined();

    String targetOrigin = valueToStringWithUndefinedOrNullCheck(exec, exec->argument((exec->argumentCount() == 2) ? 1 : 2));
    if (exec->hadException())
        return jsUndefined();

    ExceptionCode ec = 0;
    impl()->postMessage(message, &messagePorts, targetOrigin, activeDOMWindow(exec), ec);
    setDOMException(exec, ec);

    return jsUndefined();
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

    impl()->addEventListener(ustringToAtomicString(exec->argument(0).toString(exec)), JSEventListener::create(asObject(listener), this, false, currentWorld(exec)), exec->argument(2).toBoolean(exec));
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

    impl()->removeEventListener(ustringToAtomicString(exec->argument(0).toString(exec)), JSEventListener::create(asObject(listener), this, false, currentWorld(exec)).get(), exec->argument(2).toBoolean(exec));
    return jsUndefined();
}

DOMWindow* toDOMWindow(JSValue value)
{
    if (!value.isObject())
        return 0;
    JSObject* object = asObject(value);
    if (object->inherits(&JSDOMWindow::s_info))
        return static_cast<JSDOMWindow*>(object)->impl();
    if (object->inherits(&JSDOMWindowShell::s_info))
        return static_cast<JSDOMWindowShell*>(object)->impl();
    return 0;
}

} // namespace WebCore
