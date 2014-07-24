/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if ENABLE(NETSCAPE_PLUGIN_API)

#import "WebNetscapePluginEventHandlerCocoa.h"

#import "WebKitSystemInterface.h"
#import "WebNetscapePluginView.h"
#import <wtf/Vector.h>

WebNetscapePluginEventHandlerCocoa::WebNetscapePluginEventHandlerCocoa(WebNetscapePluginView* pluginView)
    : WebNetscapePluginEventHandler(pluginView)
#ifndef __LP64__
    , m_keyEventHandler(0)
#endif
{
}

static inline void initializeEvent(NPCocoaEvent* event, NPCocoaEventType type)
{
    event->type = type;
    event->version = 0;
}

void WebNetscapePluginEventHandlerCocoa::drawRect(CGContextRef context, const NSRect& rect)
{
    NPCocoaEvent event;
    
    initializeEvent(&event, NPCocoaEventDrawRect);
    event.data.draw.context = context;
    event.data.draw.x = rect.origin.x;
    event.data.draw.y = rect.origin.y;
    event.data.draw.width = rect.size.width;
    event.data.draw.height = rect.size.height;
    
    RetainPtr<CGContextRef> protect(context);
    
    sendEvent(&event);
}

void WebNetscapePluginEventHandlerCocoa::mouseDown(NSEvent *event)
{
    sendMouseEvent(event, NPCocoaEventMouseDown);
}

void WebNetscapePluginEventHandlerCocoa::mouseDragged(NSEvent *event)
{
    sendMouseEvent(event, NPCocoaEventMouseDragged);
}

void WebNetscapePluginEventHandlerCocoa::mouseEntered(NSEvent *event)
{
    sendMouseEvent(event, NPCocoaEventMouseEntered);
}

void WebNetscapePluginEventHandlerCocoa::mouseExited(NSEvent *event)
{
    sendMouseEvent(event, NPCocoaEventMouseExited);
}

void WebNetscapePluginEventHandlerCocoa::mouseMoved(NSEvent *event)
{
    sendMouseEvent(event, NPCocoaEventMouseMoved);
}

void WebNetscapePluginEventHandlerCocoa::mouseUp(NSEvent *event)
{
    sendMouseEvent(event, NPCocoaEventMouseUp);
}

bool WebNetscapePluginEventHandlerCocoa::scrollWheel(NSEvent* event)
{
    return sendMouseEvent(event, NPCocoaEventScrollWheel);
}

bool WebNetscapePluginEventHandlerCocoa::sendMouseEvent(NSEvent *nsEvent, NPCocoaEventType type)
{
    NPCocoaEvent event;
    
    NSPoint point = [m_pluginView convertPoint:[nsEvent locationInWindow] fromView:nil];
    
    int clickCount;
    if (type == NPCocoaEventMouseEntered || type == NPCocoaEventMouseExited || type == NPCocoaEventScrollWheel)
        clickCount = 0;
    else
        clickCount = [nsEvent clickCount];
    
    initializeEvent(&event, type);
    event.data.mouse.modifierFlags = [nsEvent modifierFlags];
    event.data.mouse.buttonNumber = [nsEvent buttonNumber];
    event.data.mouse.clickCount = clickCount;
    event.data.mouse.pluginX = point.x;
    event.data.mouse.pluginY = point.y;
    event.data.mouse.deltaX = [nsEvent deltaX];
    event.data.mouse.deltaY = [nsEvent deltaY];
    event.data.mouse.deltaZ = [nsEvent deltaZ];
    
    return sendEvent(&event);
}

void WebNetscapePluginEventHandlerCocoa::keyDown(NSEvent *event)
{
    bool retval = sendKeyEvent(event, NPCocoaEventKeyDown);
    
#ifndef __LP64__
    // If the plug-in did not handle the event, pass it on to the Input Manager.
    if (retval)
        WKSendKeyEventToTSM(event);
#else
    UNUSED_PARAM(retval);
#endif
}

void WebNetscapePluginEventHandlerCocoa::keyUp(NSEvent *event)
{
    sendKeyEvent(event, NPCocoaEventKeyUp);
}

void WebNetscapePluginEventHandlerCocoa::flagsChanged(NSEvent *nsEvent)
{
    NPCocoaEvent event;
        
    initializeEvent(&event, NPCocoaEventFlagsChanged);
    event.data.key.modifierFlags = [nsEvent modifierFlags];
    event.data.key.keyCode = [nsEvent keyCode];
    event.data.key.isARepeat = false;
    event.data.key.characters = 0;
    event.data.key.charactersIgnoringModifiers = 0;
    
    sendEvent(&event);
}

void WebNetscapePluginEventHandlerCocoa::syntheticKeyDownWithCommandModifier(int keyCode, char character)
{
    char nullTerminatedString[] = { character, '\0' };
    
    RetainPtr<NSString> characters = adoptNS([[NSString alloc] initWithUTF8String:nullTerminatedString]);
    
    NPCocoaEvent event;
    initializeEvent(&event, NPCocoaEventKeyDown);
    event.data.key.modifierFlags = NSCommandKeyMask;
    event.data.key.keyCode = keyCode;
    event.data.key.isARepeat = false;
    event.data.key.characters = (NPNSString *)characters.get();
    event.data.key.charactersIgnoringModifiers = (NPNSString *)characters.get();

    sendEvent(&event);
}

bool WebNetscapePluginEventHandlerCocoa::sendKeyEvent(NSEvent* nsEvent, NPCocoaEventType type)
{
    NPCocoaEvent event;

    initializeEvent(&event, type);
    event.data.key.modifierFlags = [nsEvent modifierFlags];
    event.data.key.keyCode = [nsEvent keyCode];
    event.data.key.isARepeat = [nsEvent isARepeat];
    event.data.key.characters = (NPNSString *)[nsEvent characters];
    event.data.key.charactersIgnoringModifiers = (NPNSString *)[nsEvent charactersIgnoringModifiers];
     
    return sendEvent(&event);
}

void WebNetscapePluginEventHandlerCocoa::windowFocusChanged(bool hasFocus)
{
    NPCocoaEvent event;
    
    initializeEvent(&event, NPCocoaEventWindowFocusChanged);
    event.data.focus.hasFocus = hasFocus;
    
    sendEvent(&event);
}

void WebNetscapePluginEventHandlerCocoa::focusChanged(bool hasFocus)
{
    NPCocoaEvent event;

    initializeEvent(&event, NPCocoaEventFocusChanged);
    event.data.focus.hasFocus = hasFocus;
    
    sendEvent(&event);
    
    if (hasFocus)
        installKeyEventHandler();
    else
        removeKeyEventHandler();
}

void* WebNetscapePluginEventHandlerCocoa::platformWindow(NSWindow* window)
{
    return window;
}

bool WebNetscapePluginEventHandlerCocoa::sendEvent(NPCocoaEvent* event)
{
    switch (event->type) {
        case NPCocoaEventMouseDown:
        case NPCocoaEventMouseUp:
        case NPCocoaEventMouseDragged:
        case NPCocoaEventKeyDown:
        case NPCocoaEventKeyUp:
        case NPCocoaEventFlagsChanged:
        case NPCocoaEventScrollWheel:
            m_currentEventIsUserGesture = true;
            break;
        default:
            m_currentEventIsUserGesture = false;
    }
            
    bool result = [m_pluginView sendEvent:event isDrawRect:event->type == NPCocoaEventDrawRect];
    
    m_currentEventIsUserGesture = false;
    return result;
}

#ifndef __LP64__

void WebNetscapePluginEventHandlerCocoa::installKeyEventHandler()
{
    static const EventTypeSpec TSMEvents[] =
    {
        { kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }
    };
    
    if (!m_keyEventHandler)
        InstallEventHandler(GetWindowEventTarget((WindowRef)[[m_pluginView window] windowRef]),
                            NewEventHandlerUPP(TSMEventHandler),
                            GetEventTypeCount(TSMEvents),
                            TSMEvents,
                            this,
                            &m_keyEventHandler);
}

void WebNetscapePluginEventHandlerCocoa::removeKeyEventHandler()
{
    if (m_keyEventHandler) {
        RemoveEventHandler(m_keyEventHandler);
        m_keyEventHandler = 0;
    }    
}

OSStatus WebNetscapePluginEventHandlerCocoa::TSMEventHandler(EventHandlerCallRef inHandlerRef, EventRef event, void* eventHandler)
{
    return static_cast<WebNetscapePluginEventHandlerCocoa*>(eventHandler)->handleTSMEvent(event);
}

OSStatus WebNetscapePluginEventHandlerCocoa::handleTSMEvent(EventRef eventRef)
{
    ASSERT(GetEventKind(eventRef) == kEventTextInputUnicodeForKeyEvent);
    
    // Get the text buffer size.
    ByteCount size;
    OSStatus result = GetEventParameter(eventRef, kEventParamTextInputSendText, typeUnicodeText, 0, 0, &size, 0);
    if (result != noErr)
        return result;
    
    unsigned length = size / sizeof(UniChar);
    Vector<UniChar, 16> characters(length);
    
    // Now get the actual text.
    result = GetEventParameter(eventRef, kEventParamTextInputSendText, typeUnicodeText, 0, size, 0, characters.data());
    if (result != noErr)
        return result;

    RetainPtr<CFStringRef> text = adoptCF(CFStringCreateWithCharacters(0, characters.data(), length));

    NPCocoaEvent event;
    
    initializeEvent(&event, NPCocoaEventTextInput);
    event.data.text.text = (NPNSString*)text.get();
    
    sendEvent(&event);

    return noErr;
}

#endif // __LP64__

#endif // ENABLE(NETSCAPE_PLUGIN_API)
