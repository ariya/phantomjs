/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#ifndef DOMEventsClasses_H
#define DOMEventsClasses_H

#include "WebKit.h"
#include "DOMCoreClasses.h"
#include <WebCore/EventListener.h>

#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

// {AC3D1BC3-4976-4431-8A19-4812C5EFE39C}
DEFINE_GUID(IID_DOMEvent, 0xac3d1bc3, 0x4976, 0x4431, 0x8a, 0x19, 0x48, 0x12, 0xc5, 0xef, 0xe3, 0x9c);

namespace WebCore {
    class Event;
}

class DOMUIEvent;

class WebEventListener : public WebCore::EventListener {
public:
    WebEventListener(IDOMEventListener*);
    ~WebEventListener();
    virtual bool operator==(const EventListener&);
    virtual void handleEvent(WebCore::ScriptExecutionContext*, WebCore::Event*);
    static PassRefPtr<WebEventListener> create(IDOMEventListener*);
private:
    IDOMEventListener* m_iDOMEventListener;
};

class DOMEventListener : public DOMObject, public IDOMEventListener
{
public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMEventListener
    virtual HRESULT STDMETHODCALLTYPE handleEvent( 
        /* [in] */ IDOMEvent* evt);
};

class DOMEvent : public DOMObject, public IDOMEvent
{
public:
    static IDOMEvent* createInstance(PassRefPtr<WebCore::Event> e);
protected:
    DOMEvent(PassRefPtr<WebCore::Event> e);
    ~DOMEvent();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMObject::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMObject::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMEvent
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE target( 
        /* [retval][out] */ IDOMEventTarget** result);
    
    virtual HRESULT STDMETHODCALLTYPE currentTarget( 
        /* [retval][out] */ IDOMEventTarget** result);
    
    virtual HRESULT STDMETHODCALLTYPE eventPhase( 
        /* [retval][out] */ unsigned short* result);
    
    virtual HRESULT STDMETHODCALLTYPE bubbles( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE cancelable( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE timeStamp( 
        /* [retval][out] */ DOMTimeStamp* result);
    
    virtual HRESULT STDMETHODCALLTYPE stopPropagation( void);
    
    virtual HRESULT STDMETHODCALLTYPE preventDefault( void);
    
    virtual HRESULT STDMETHODCALLTYPE initEvent( 
        /* [in] */ BSTR eventTypeArg,
        /* [in] */ BOOL canBubbleArg,
        /* [in] */ BOOL cancelableArg);

    // DOMEvent methods
    WebCore::Event* coreEvent() { return m_event.get(); }

protected:
    RefPtr<WebCore::Event> m_event;
};

class DOMUIEvent : public DOMEvent, public IDOMUIEvent
{
public:
    DOMUIEvent(PassRefPtr<WebCore::Event> e) : DOMEvent(e) {}

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMEvent::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMEvent::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMEvent
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR* result) { return DOMEvent::type(result); }
    
    virtual HRESULT STDMETHODCALLTYPE target( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::target(result); }
    
    virtual HRESULT STDMETHODCALLTYPE currentTarget( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::currentTarget(result); }
    
    virtual HRESULT STDMETHODCALLTYPE eventPhase( 
        /* [retval][out] */ unsigned short* result) { return DOMEvent::eventPhase(result); }
    
    virtual HRESULT STDMETHODCALLTYPE bubbles( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::bubbles(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cancelable( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::cancelable(result); }
    
    virtual HRESULT STDMETHODCALLTYPE timeStamp( 
        /* [retval][out] */ DOMTimeStamp* result) { return DOMEvent::timeStamp(result); }
    
    virtual HRESULT STDMETHODCALLTYPE stopPropagation( void) { return DOMEvent::stopPropagation(); }
    
    virtual HRESULT STDMETHODCALLTYPE preventDefault( void) { return DOMEvent::preventDefault(); }
    
    virtual HRESULT STDMETHODCALLTYPE initEvent( 
        /* [in] */ BSTR eventTypeArg,
        /* [in] */ BOOL canBubbleArg,
        /* [in] */ BOOL cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // IDOMUIEvent
    virtual HRESULT STDMETHODCALLTYPE view( 
        /* [retval][out] */ IDOMWindow** result);
    
    virtual HRESULT STDMETHODCALLTYPE detail( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE initUIEvent( 
        /* [in] */ BSTR type,
        /* [in] */ BOOL canBubble,
        /* [in] */ BOOL cancelable,
        /* [in] */ IDOMWindow* view,
        /* [in] */ long detail);
    
    virtual HRESULT STDMETHODCALLTYPE keyCode( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE charCode( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE unused1(
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE unused2(
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE pageX( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE pageY( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE which( 
        /* [retval][out] */ long* result);
};

class DOMKeyboardEvent : public DOMUIEvent, public IDOMKeyboardEvent
{
public:
    DOMKeyboardEvent(PassRefPtr<WebCore::Event> e) : DOMUIEvent(e) { }

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMUIEvent::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMUIEvent::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMEvent
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR* result) { return DOMEvent::type(result); }
    
    virtual HRESULT STDMETHODCALLTYPE target( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::target(result); }
    
    virtual HRESULT STDMETHODCALLTYPE currentTarget( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::currentTarget(result); }
    
    virtual HRESULT STDMETHODCALLTYPE eventPhase( 
        /* [retval][out] */ unsigned short* result) { return DOMEvent::eventPhase(result); }
    
    virtual HRESULT STDMETHODCALLTYPE bubbles( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::bubbles(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cancelable( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::cancelable(result); }
    
    virtual HRESULT STDMETHODCALLTYPE timeStamp( 
        /* [retval][out] */ DOMTimeStamp* result) { return DOMEvent::timeStamp(result); }
    
    virtual HRESULT STDMETHODCALLTYPE stopPropagation( void) { return DOMEvent::stopPropagation(); }
    
    virtual HRESULT STDMETHODCALLTYPE preventDefault( void) { return DOMEvent::preventDefault(); }
    
    virtual HRESULT STDMETHODCALLTYPE initEvent( 
        /* [in] */ BSTR eventTypeArg,
        /* [in] */ BOOL canBubbleArg,
        /* [in] */ BOOL cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // IDOMUIEvent
    virtual HRESULT STDMETHODCALLTYPE view( 
        /* [retval][out] */ IDOMWindow** result) { return DOMUIEvent::view(result); }
    
    virtual HRESULT STDMETHODCALLTYPE detail( 
        /* [retval][out] */ long* result) { return DOMUIEvent::detail(result); }
    
    virtual HRESULT STDMETHODCALLTYPE initUIEvent( 
        /* [in] */ BSTR type,
        /* [in] */ BOOL canBubble,
        /* [in] */ BOOL cancelable,
        /* [in] */ IDOMWindow* view,
        /* [in] */ long detail) { return DOMUIEvent::initUIEvent(type, canBubble, cancelable, view, detail); }
    
    virtual HRESULT STDMETHODCALLTYPE keyCode( 
        /* [retval][out] */ long* result) { return DOMUIEvent::keyCode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE charCode( 
        /* [retval][out] */ long* result) { return DOMUIEvent::charCode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE unused1(
        /* [retval][out] */ long* result) { return DOMUIEvent::unused1(result); }
    
    virtual HRESULT STDMETHODCALLTYPE unused2(
        /* [retval][out] */ long* result) { return DOMUIEvent::unused2(result); }
    
    virtual HRESULT STDMETHODCALLTYPE pageX( 
        /* [retval][out] */ long* result) { return DOMUIEvent::pageX(result); }
    
    virtual HRESULT STDMETHODCALLTYPE pageY( 
        /* [retval][out] */ long* result) { return DOMUIEvent::pageY(result); }
    
    virtual HRESULT STDMETHODCALLTYPE which( 
        /* [retval][out] */ long* result) { return DOMUIEvent::which(result); }

    // IDOMKeyboardEvent
    virtual HRESULT STDMETHODCALLTYPE keyIdentifier( 
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE location(
        /* [retval][out] */ unsigned long* result);
    
    virtual HRESULT STDMETHODCALLTYPE keyLocation( 
        /* [retval][out] */ unsigned long* result);
    
    virtual HRESULT STDMETHODCALLTYPE ctrlKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE shiftKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE altKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE metaKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE altGraphKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE getModifierState( 
        /* [in] */ BSTR keyIdentifierArg,
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE initKeyboardEvent( 
        /* [in] */ BSTR type,
        /* [in] */ BOOL canBubble,
        /* [in] */ BOOL cancelable,
        /* [in] */ IDOMWindow* view,
        /* [in] */ BSTR keyIdentifier,
        /* [in] */ unsigned long keyLocation,
        /* [in] */ BOOL ctrlKey,
        /* [in] */ BOOL altKey,
        /* [in] */ BOOL shiftKey,
        /* [in] */ BOOL metaKey,
        /* [in] */ BOOL graphKey);
};

class DOMMouseEvent : public DOMUIEvent, public IDOMMouseEvent
{
public:
    DOMMouseEvent(PassRefPtr<WebCore::Event> e) : DOMUIEvent(e) { }

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMUIEvent::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMUIEvent::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMEvent
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR* result) { return DOMEvent::type(result); }
    
    virtual HRESULT STDMETHODCALLTYPE target( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::target(result); }
    
    virtual HRESULT STDMETHODCALLTYPE currentTarget( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::currentTarget(result); }
    
    virtual HRESULT STDMETHODCALLTYPE eventPhase( 
        /* [retval][out] */ unsigned short* result) { return DOMEvent::eventPhase(result); }
    
    virtual HRESULT STDMETHODCALLTYPE bubbles( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::bubbles(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cancelable( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::cancelable(result); }
    
    virtual HRESULT STDMETHODCALLTYPE timeStamp( 
        /* [retval][out] */ DOMTimeStamp* result) { return DOMEvent::timeStamp(result); }
    
    virtual HRESULT STDMETHODCALLTYPE stopPropagation( void) { return DOMEvent::stopPropagation(); }
    
    virtual HRESULT STDMETHODCALLTYPE preventDefault( void) { return DOMEvent::preventDefault(); }
    
    virtual HRESULT STDMETHODCALLTYPE initEvent( 
        /* [in] */ BSTR eventTypeArg,
        /* [in] */ BOOL canBubbleArg,
        /* [in] */ BOOL cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // IDOMUIEvent
    virtual HRESULT STDMETHODCALLTYPE view( 
        /* [retval][out] */ IDOMWindow** result) { return DOMUIEvent::view(result); }
    
    virtual HRESULT STDMETHODCALLTYPE detail( 
        /* [retval][out] */ long* result) { return DOMUIEvent::detail(result); }
    
    virtual HRESULT STDMETHODCALLTYPE initUIEvent( 
        /* [in] */ BSTR type,
        /* [in] */ BOOL canBubble,
        /* [in] */ BOOL cancelable,
        /* [in] */ IDOMWindow* view,
        /* [in] */ long detail) { return DOMUIEvent::initUIEvent(type, canBubble, cancelable, view, detail); }
    
    virtual HRESULT STDMETHODCALLTYPE keyCode( 
        /* [retval][out] */ long* result) { return DOMUIEvent::keyCode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE charCode( 
        /* [retval][out] */ long* result) { return DOMUIEvent::charCode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE unused1(
        /* [retval][out] */ long* result) { return DOMUIEvent::unused1(result); }
    
    virtual HRESULT STDMETHODCALLTYPE unused2(
        /* [retval][out] */ long* result) { return DOMUIEvent::unused2(result); }
    
    virtual HRESULT STDMETHODCALLTYPE pageX( 
        /* [retval][out] */ long* result) { return DOMUIEvent::pageX(result); }
    
    virtual HRESULT STDMETHODCALLTYPE pageY( 
        /* [retval][out] */ long* result) { return DOMUIEvent::pageY(result); }
    
    virtual HRESULT STDMETHODCALLTYPE which( 
        /* [retval][out] */ long* result) { return DOMUIEvent::which(result); }

    // IDOMMouseEvent
    virtual HRESULT STDMETHODCALLTYPE screenX( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE screenY( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE clientX( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE clientY( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE ctrlKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE shiftKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE altKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE metaKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE button( 
        /* [retval][out] */ unsigned short* result);
    
    virtual HRESULT STDMETHODCALLTYPE relatedTarget( 
        /* [retval][out] */ IDOMEventTarget** result);
    
    virtual HRESULT STDMETHODCALLTYPE initMouseEvent( 
        /* [in] */ BSTR type,
        /* [in] */ BOOL canBubble,
        /* [in] */ BOOL cancelable,
        /* [in] */ IDOMWindow* view,
        /* [in] */ long detail,
        /* [in] */ long screenX,
        /* [in] */ long screenY,
        /* [in] */ long clientX,
        /* [in] */ long clientY,
        /* [in] */ BOOL ctrlKey,
        /* [in] */ BOOL altKey,
        /* [in] */ BOOL shiftKey,
        /* [in] */ BOOL metaKey,
        /* [in] */ unsigned short button,
        /* [in] */ IDOMEventTarget* relatedTarget);
    
    virtual HRESULT STDMETHODCALLTYPE offsetX( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE offsetY( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE x( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE y( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE fromElement( 
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE toElement( 
        /* [retval][out] */ IDOMNode** result);
};

class DOMMutationEvent : public DOMEvent, public IDOMMutationEvent
{
public:
    DOMMutationEvent(PassRefPtr<WebCore::Event> e) : DOMEvent(e) { }

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMEvent::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMEvent::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMEvent
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR* result) { return DOMEvent::type(result); }
    
    virtual HRESULT STDMETHODCALLTYPE target( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::target(result); }
    
    virtual HRESULT STDMETHODCALLTYPE currentTarget( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::currentTarget(result); }
    
    virtual HRESULT STDMETHODCALLTYPE eventPhase( 
        /* [retval][out] */ unsigned short* result) { return DOMEvent::eventPhase(result); }
    
    virtual HRESULT STDMETHODCALLTYPE bubbles( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::bubbles(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cancelable( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::cancelable(result); }
    
    virtual HRESULT STDMETHODCALLTYPE timeStamp( 
        /* [retval][out] */ DOMTimeStamp* result) { return DOMEvent::timeStamp(result); }
    
    virtual HRESULT STDMETHODCALLTYPE stopPropagation( void) { return DOMEvent::stopPropagation(); }
    
    virtual HRESULT STDMETHODCALLTYPE preventDefault( void) { return DOMEvent::preventDefault(); }
    
    virtual HRESULT STDMETHODCALLTYPE initEvent( 
        /* [in] */ BSTR eventTypeArg,
        /* [in] */ BOOL canBubbleArg,
        /* [in] */ BOOL cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // IDOMMutationEvent
    virtual HRESULT STDMETHODCALLTYPE relatedNode( 
        /* [retval][out] */ IDOMNode** result);
    
    virtual HRESULT STDMETHODCALLTYPE prevValue( 
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE newValue( 
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE attrName( 
        /* [retval][out] */ BSTR* result);
    
    virtual HRESULT STDMETHODCALLTYPE attrChange( 
        /* [retval][out] */ unsigned short* result);
    
    virtual HRESULT STDMETHODCALLTYPE initMutationEvent( 
        /* [in] */ BSTR type,
        /* [in] */ BOOL canBubble,
        /* [in] */ BOOL cancelable,
        /* [in] */ IDOMNode* relatedNode,
        /* [in] */ BSTR prevValue,
        /* [in] */ BSTR newValue,
        /* [in] */ BSTR attrName,
        /* [in] */ unsigned short attrChange);
};

class DOMOverflowEvent : public DOMEvent, public IDOMOverflowEvent
{
public:
    DOMOverflowEvent(PassRefPtr<WebCore::Event> e) : DOMEvent(e) { }

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMEvent::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMEvent::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMEvent
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR* result) { return DOMEvent::type(result); }
    
    virtual HRESULT STDMETHODCALLTYPE target( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::target(result); }
    
    virtual HRESULT STDMETHODCALLTYPE currentTarget( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::currentTarget(result); }
    
    virtual HRESULT STDMETHODCALLTYPE eventPhase( 
        /* [retval][out] */ unsigned short* result) { return DOMEvent::eventPhase(result); }
    
    virtual HRESULT STDMETHODCALLTYPE bubbles( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::bubbles(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cancelable( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::cancelable(result); }
    
    virtual HRESULT STDMETHODCALLTYPE timeStamp( 
        /* [retval][out] */ DOMTimeStamp* result) { return DOMEvent::timeStamp(result); }
    
    virtual HRESULT STDMETHODCALLTYPE stopPropagation( void) { return DOMEvent::stopPropagation(); }
    
    virtual HRESULT STDMETHODCALLTYPE preventDefault( void) { return DOMEvent::preventDefault(); }
    
    virtual HRESULT STDMETHODCALLTYPE initEvent( 
        /* [in] */ BSTR eventTypeArg,
        /* [in] */ BOOL canBubbleArg,
        /* [in] */ BOOL cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // IDOMOverflowEvent
    virtual HRESULT STDMETHODCALLTYPE orient( 
        /* [retval][out] */ unsigned short* result);
    
    virtual HRESULT STDMETHODCALLTYPE horizontalOverflow( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE verticalOverflow( 
        /* [retval][out] */ BOOL* result);
};

class DOMWheelEvent : public DOMUIEvent, public IDOMWheelEvent
{
public:
    DOMWheelEvent(PassRefPtr<WebCore::Event> e) : DOMUIEvent(e) { }

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return DOMUIEvent::AddRef(); }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return DOMUIEvent::Release(); }

    // IWebScriptObject
    virtual HRESULT STDMETHODCALLTYPE throwException( 
        /* [in] */ BSTR exceptionMessage,
        /* [retval][out] */ BOOL* result) { return DOMObject::throwException(exceptionMessage, result); }
    
    virtual HRESULT STDMETHODCALLTYPE callWebScriptMethod( 
        /* [in] */ BSTR name,
        /* [size_is][in] */ const VARIANT args[  ],
        /* [in] */ int cArgs,
        /* [retval][out] */ VARIANT* result) { return DOMObject::callWebScriptMethod(name, args, cArgs, result); }
    
    virtual HRESULT STDMETHODCALLTYPE evaluateWebScript( 
        /* [in] */ BSTR script,
        /* [retval][out] */ VARIANT* result) { return DOMObject::evaluateWebScript(script, result); }
    
    virtual HRESULT STDMETHODCALLTYPE removeWebScriptKey( 
        /* [in] */ BSTR name) { return DOMObject::removeWebScriptKey(name); }
    
    virtual HRESULT STDMETHODCALLTYPE stringRepresentation( 
        /* [retval][out] */ BSTR* stringRepresentation) { return DOMObject::stringRepresentation(stringRepresentation); }
    
    virtual HRESULT STDMETHODCALLTYPE webScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [retval][out] */ VARIANT* result) { return DOMObject::webScriptValueAtIndex(index, result); }
    
    virtual HRESULT STDMETHODCALLTYPE setWebScriptValueAtIndex( 
        /* [in] */ unsigned int index,
        /* [in] */ VARIANT val) { return DOMObject::setWebScriptValueAtIndex(index, val); }
    
    virtual HRESULT STDMETHODCALLTYPE setException( 
        /* [in] */ BSTR description) { return DOMObject::setException(description); }

    // IDOMEvent
    virtual HRESULT STDMETHODCALLTYPE type( 
        /* [retval][out] */ BSTR* result) { return DOMEvent::type(result); }
    
    virtual HRESULT STDMETHODCALLTYPE target( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::target(result); }
    
    virtual HRESULT STDMETHODCALLTYPE currentTarget( 
        /* [retval][out] */ IDOMEventTarget** result) { return DOMEvent::currentTarget(result); }
    
    virtual HRESULT STDMETHODCALLTYPE eventPhase( 
        /* [retval][out] */ unsigned short* result) { return DOMEvent::eventPhase(result); }
    
    virtual HRESULT STDMETHODCALLTYPE bubbles( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::bubbles(result); }
    
    virtual HRESULT STDMETHODCALLTYPE cancelable( 
        /* [retval][out] */ BOOL* result) { return DOMEvent::cancelable(result); }
    
    virtual HRESULT STDMETHODCALLTYPE timeStamp( 
        /* [retval][out] */ DOMTimeStamp* result) { return DOMEvent::timeStamp(result); }
    
    virtual HRESULT STDMETHODCALLTYPE stopPropagation( void) { return DOMEvent::stopPropagation(); }
    
    virtual HRESULT STDMETHODCALLTYPE preventDefault( void) { return DOMEvent::preventDefault(); }
    
    virtual HRESULT STDMETHODCALLTYPE initEvent( 
        /* [in] */ BSTR eventTypeArg,
        /* [in] */ BOOL canBubbleArg,
        /* [in] */ BOOL cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // IDOMUIEvent
    virtual HRESULT STDMETHODCALLTYPE view( 
        /* [retval][out] */ IDOMWindow** result) { return DOMUIEvent::view(result); }
    
    virtual HRESULT STDMETHODCALLTYPE detail( 
        /* [retval][out] */ long* result) { return DOMUIEvent::detail(result); }
    
    virtual HRESULT STDMETHODCALLTYPE initUIEvent( 
        /* [in] */ BSTR type,
        /* [in] */ BOOL canBubble,
        /* [in] */ BOOL cancelable,
        /* [in] */ IDOMWindow* view,
        /* [in] */ long detail) { return DOMUIEvent::initUIEvent(type, canBubble, cancelable, view, detail); }
    
    virtual HRESULT STDMETHODCALLTYPE keyCode( 
        /* [retval][out] */ long* result) { return DOMUIEvent::keyCode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE charCode( 
        /* [retval][out] */ long* result) { return DOMUIEvent::charCode(result); }
    
    virtual HRESULT STDMETHODCALLTYPE unused1(
        /* [retval][out] */ long* result) { return DOMUIEvent::unused1(result); }
    
    virtual HRESULT STDMETHODCALLTYPE unused2(
        /* [retval][out] */ long* result) { return DOMUIEvent::unused2(result); }
    
    virtual HRESULT STDMETHODCALLTYPE pageX( 
        /* [retval][out] */ long* result) { return DOMUIEvent::pageX(result); }
    
    virtual HRESULT STDMETHODCALLTYPE pageY( 
        /* [retval][out] */ long* result) { return DOMUIEvent::pageY(result); }
    
    virtual HRESULT STDMETHODCALLTYPE which( 
        /* [retval][out] */ long* result) { return DOMUIEvent::which(result); }

    // IDOMWheelEvent
    virtual HRESULT STDMETHODCALLTYPE screenX( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE screenY( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE clientX( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE clientY( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE ctrlKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE shiftKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE altKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE metaKey( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE wheelDelta( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE wheelDeltaX( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE wheelDeltaY( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE offsetX( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE offsetY( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE x( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE y( 
        /* [retval][out] */ long* result);
    
    virtual HRESULT STDMETHODCALLTYPE isHorizontal( 
        /* [retval][out] */ BOOL* result);
    
    virtual HRESULT STDMETHODCALLTYPE initWheelEvent( 
        /* [in] */ long wheelDeltaX,
        /* [in] */ long wheelDeltaY,
        /* [in] */ IDOMWindow* view,
        /* [in] */ long screenX,
        /* [in] */ long screenY,
        /* [in] */ long clientX,
        /* [in] */ long clientY,
        /* [in] */ BOOL ctrlKey,
        /* [in] */ BOOL altKey,
        /* [in] */ BOOL shiftKey,
        /* [in] */ BOOL metaKey);
};

#endif
