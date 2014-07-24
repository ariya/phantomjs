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

#if ENABLE(NETSCAPE_PLUGIN_API) && !defined(__LP64__)

#import "WebNetscapePluginEventHandlerCarbon.h"

#import "WebNetscapePluginView.h"
#import "WebKitLogging.h"
#import "WebKitSystemInterface.h"

// Send null events 50 times a second when active, so plug-ins like Flash get high frame rates.
#define NullEventIntervalActive         0.02
#define NullEventIntervalNotActive      0.25

WebNetscapePluginEventHandlerCarbon::WebNetscapePluginEventHandlerCarbon(WebNetscapePluginView* pluginView)
    : WebNetscapePluginEventHandler(pluginView)
    , m_keyEventHandler(0)
    , m_suspendKeyUpEvents(false)
{
}

static void getCarbonEvent(EventRecord* carbonEvent)
{
    carbonEvent->what = nullEvent;
    carbonEvent->message = 0;
    carbonEvent->when = TickCount();
    
    GetGlobalMouse(&carbonEvent->where);
    carbonEvent->modifiers = GetCurrentKeyModifiers();
    if (!Button())
        carbonEvent->modifiers |= btnState;
}

static EventModifiers modifiersForEvent(NSEvent *event)
{
    EventModifiers modifiers;
    unsigned int modifierFlags = [event modifierFlags];
    NSEventType eventType = [event type];
    
    modifiers = 0;
    
    if (eventType != NSLeftMouseDown && eventType != NSRightMouseDown)
        modifiers |= btnState;
    
    if (modifierFlags & NSCommandKeyMask)
        modifiers |= cmdKey;
    
    if (modifierFlags & NSShiftKeyMask)
        modifiers |= shiftKey;

    if (modifierFlags & NSAlphaShiftKeyMask)
        modifiers |= alphaLock;

    if (modifierFlags & NSAlternateKeyMask)
        modifiers |= optionKey;

    if (modifierFlags & NSControlKeyMask || eventType == NSRightMouseDown)
        modifiers |= controlKey;
    
    return modifiers;
}

static void getCarbonEvent(EventRecord *carbonEvent, NSEvent *cocoaEvent)
{
    if (WKConvertNSEventToCarbonEvent(carbonEvent, cocoaEvent))
        return;
    
    NSPoint where = [[cocoaEvent window] convertBaseToScreen:[cocoaEvent locationInWindow]];
        
    carbonEvent->what = nullEvent;
    carbonEvent->message = 0;
    carbonEvent->when = (UInt32)([cocoaEvent timestamp] * 60); // seconds to ticks
    carbonEvent->where.h = (short)where.x;
    carbonEvent->where.v = (short)(NSMaxY([(NSScreen *)[[NSScreen screens] objectAtIndex:0] frame]) - where.y);
    carbonEvent->modifiers = modifiersForEvent(cocoaEvent);
}

void WebNetscapePluginEventHandlerCarbon::sendNullEvent()
{
    EventRecord event;
    
    getCarbonEvent(&event);
    
    // Plug-in should not react to cursor position when not active or when a menu is down.
    MenuTrackingData trackingData;
    OSStatus error = GetMenuTrackingData(NULL, &trackingData);
    
    // Plug-in should not react to cursor position when the actual window is not key.
    if (![[m_pluginView window] isKeyWindow] || (error == noErr && trackingData.menu)) {
        // FIXME: Does passing a v and h of -1 really prevent it from reacting to the cursor position?
        event.where.v = -1;
        event.where.h = -1;
    }
    
    sendEvent(&event);
}

void WebNetscapePluginEventHandlerCarbon::drawRect(CGContextRef, const NSRect&)
{
    EventRecord event;
    
    getCarbonEvent(&event);
    event.what = updateEvt;
    WindowRef windowRef = (WindowRef)[[m_pluginView window] windowRef];
    event.message = (unsigned long)windowRef;
    
    BOOL acceptedEvent;
    acceptedEvent = sendEvent(&event);
    
    LOG(PluginEvents, "NPP_HandleEvent(updateEvt): %d", acceptedEvent);
}

void WebNetscapePluginEventHandlerCarbon::mouseDown(NSEvent* theEvent)
{
    EventRecord event;
    
    getCarbonEvent(&event, theEvent);
    event.what = ::mouseDown;
    
    BOOL acceptedEvent;
    acceptedEvent = sendEvent(&event);
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseDown): %d pt.v=%d, pt.h=%d", acceptedEvent, event.where.v, event.where.h);    
}

void WebNetscapePluginEventHandlerCarbon::mouseUp(NSEvent* theEvent)
{
    EventRecord event;
    
    getCarbonEvent(&event, theEvent);
    event.what = ::mouseUp;
    
    BOOL acceptedEvent;
    acceptedEvent = sendEvent(&event);
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseUp): %d pt.v=%d, pt.h=%d", acceptedEvent, event.where.v, event.where.h);    
}

bool WebNetscapePluginEventHandlerCarbon::scrollWheel(NSEvent* theEvent)
{
    return false;
}

void WebNetscapePluginEventHandlerCarbon::mouseEntered(NSEvent* theEvent)
{
    EventRecord event;
    
    getCarbonEvent(&event, theEvent);
    event.what = NPEventType_AdjustCursorEvent;
    
    BOOL acceptedEvent;
    acceptedEvent = sendEvent(&event);
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseEntered): %d", acceptedEvent);    
}

void WebNetscapePluginEventHandlerCarbon::mouseExited(NSEvent* theEvent)
{
    EventRecord event;
    
    getCarbonEvent(&event, theEvent);
    event.what = NPEventType_AdjustCursorEvent;
    
    BOOL acceptedEvent;
    acceptedEvent = sendEvent(&event);
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseExited): %d", acceptedEvent);    
}

void WebNetscapePluginEventHandlerCarbon::mouseDragged(NSEvent*)
{
}

void WebNetscapePluginEventHandlerCarbon::mouseMoved(NSEvent* theEvent)
{
    EventRecord event;
    
    getCarbonEvent(&event, theEvent);
    event.what = NPEventType_AdjustCursorEvent;
    
    BOOL acceptedEvent;
    acceptedEvent = sendEvent(&event);
    
    LOG(PluginEvents, "NPP_HandleEvent(mouseMoved): %d", acceptedEvent);
}

void WebNetscapePluginEventHandlerCarbon::keyDown(NSEvent *theEvent)
{
    m_suspendKeyUpEvents = true;
    WKSendKeyEventToTSM(theEvent);
}

void WebNetscapePluginEventHandlerCarbon::syntheticKeyDownWithCommandModifier(int keyCode, char character)
{
    EventRecord event;
    getCarbonEvent(&event);
    
    event.what = ::keyDown;
    event.modifiers |= cmdKey;
    event.message = keyCode << 8 | character;
    sendEvent(&event);
}

static UInt32 keyMessageForEvent(NSEvent *event)
{
    NSData *data = [[event characters] dataUsingEncoding:CFStringConvertEncodingToNSStringEncoding(CFStringGetSystemEncoding())];
    if (!data)
        return 0;

    UInt8 characterCode;
    [data getBytes:&characterCode length:1];
    UInt16 keyCode = [event keyCode];
    return keyCode << 8 | characterCode;
}    
    
void WebNetscapePluginEventHandlerCarbon::keyUp(NSEvent* theEvent)
{
    WKSendKeyEventToTSM(theEvent);
    
    // TSM won't send keyUp events so we have to send them ourselves.
    // Only send keyUp events after we receive the TSM callback because this is what plug-in expect from OS 9.
    if (!m_suspendKeyUpEvents) {
        EventRecord event;
        
        getCarbonEvent(&event, theEvent);
        event.what = ::keyUp;
        
        if (event.message == 0)
            event.message = keyMessageForEvent(theEvent);
        
        sendEvent(&event);
    }    
}

void WebNetscapePluginEventHandlerCarbon::flagsChanged(NSEvent*)
{
}

void WebNetscapePluginEventHandlerCarbon::focusChanged(bool hasFocus)
{
    EventRecord event;
    
    getCarbonEvent(&event);
    bool acceptedEvent;
    if (hasFocus) {
        event.what = NPEventType_GetFocusEvent;
        acceptedEvent = sendEvent(&event);
        LOG(PluginEvents, "NPP_HandleEvent(NPEventType_GetFocusEvent): %d", acceptedEvent);
        installKeyEventHandler();
    } else {
        event.what = NPEventType_LoseFocusEvent;
        acceptedEvent = sendEvent(&event);
        LOG(PluginEvents, "NPP_HandleEvent(NPEventType_LoseFocusEvent): %d", acceptedEvent);
        removeKeyEventHandler();
    }
}

void WebNetscapePluginEventHandlerCarbon::windowFocusChanged(bool hasFocus)
{
    WindowRef windowRef = (WindowRef)[[m_pluginView window] windowRef];

    SetUserFocusWindow(windowRef);
    
    EventRecord event;
    
    getCarbonEvent(&event);
    event.what = activateEvt;
    event.message = (unsigned long)windowRef;
    if (hasFocus)
        event.modifiers |= activeFlag;
    
    BOOL acceptedEvent;
    acceptedEvent = sendEvent(&event);
    
    LOG(PluginEvents, "NPP_HandleEvent(activateEvent): %d  isActive: %d", acceptedEvent, hasFocus);    
}

OSStatus WebNetscapePluginEventHandlerCarbon::TSMEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *eventHandler)
{    
    EventRef rawKeyEventRef;
    OSStatus status = GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, sizeof(EventRef), NULL, &rawKeyEventRef);
    if (status != noErr) {
        LOG_ERROR("GetEventParameter failed with error: %d", status);
        return noErr;
    }
    
    // Two-pass read to allocate/extract Mac charCodes
    ByteCount numBytes;    
    status = GetEventParameter(rawKeyEventRef, kEventParamKeyMacCharCodes, typeChar, NULL, 0, &numBytes, NULL);
    if (status != noErr) {
        LOG_ERROR("GetEventParameter failed with error: %d", status);
        return noErr;
    }
    char *buffer = (char *)malloc(numBytes);
    status = GetEventParameter(rawKeyEventRef, kEventParamKeyMacCharCodes, typeChar, NULL, numBytes, NULL, buffer);
    if (status != noErr) {
        LOG_ERROR("GetEventParameter failed with error: %d", status);
        free(buffer);
        return noErr;
    }
    
    EventRef cloneEvent = CopyEvent(rawKeyEventRef);
    unsigned i;
    for (i = 0; i < numBytes; i++) {
        status = SetEventParameter(cloneEvent, kEventParamKeyMacCharCodes, typeChar, 1 /* one char code */, &buffer[i]);
        if (status != noErr) {
            LOG_ERROR("SetEventParameter failed with error: %d", status);
            free(buffer);
            return noErr;
        }
        
        EventRecord eventRec;
        if (ConvertEventRefToEventRecord(cloneEvent, &eventRec)) {
            BOOL acceptedEvent;
            acceptedEvent = static_cast<WebNetscapePluginEventHandlerCarbon*>(eventHandler)->sendEvent(&eventRec);
            
            LOG(PluginEvents, "NPP_HandleEvent(keyDown): %d charCode:%c keyCode:%lu",
                acceptedEvent, (char) (eventRec.message & charCodeMask), (eventRec.message & keyCodeMask));
            
            // We originally thought that if the plug-in didn't accept this event,
            // we should pass it along so that keyboard scrolling, for example, will work.
            // In practice, this is not a good idea, because plug-ins tend to eat the event but return false.
            // MacIE handles each key event twice because of this, but we will emulate the other browsers instead.
        }
    }
    ReleaseEvent(cloneEvent);
    
    free(buffer);
    
    return noErr;
}

void WebNetscapePluginEventHandlerCarbon::installKeyEventHandler()
{
    static const EventTypeSpec sTSMEvents[] =
    {
        { kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }
    };
    
    if (!m_keyEventHandler) {
        InstallEventHandler(GetWindowEventTarget((WindowRef)[[m_pluginView window] windowRef]),
                            NewEventHandlerUPP(TSMEventHandler),
                            GetEventTypeCount(sTSMEvents),
                            sTSMEvents,
                            this,
                            &m_keyEventHandler);
    }
}

void WebNetscapePluginEventHandlerCarbon::removeKeyEventHandler()
{
    if (m_keyEventHandler) {
        RemoveEventHandler(m_keyEventHandler);
        m_keyEventHandler = 0;
    }    
}

void WebNetscapePluginEventHandlerCarbon::nullEventTimerFired(CFRunLoopTimerRef timerRef, void *context)
{
    static_cast<WebNetscapePluginEventHandlerCarbon*>(context)->sendNullEvent();
}

void WebNetscapePluginEventHandlerCarbon::startTimers(bool throttleTimers)
{
    ASSERT(!m_nullEventTimer);
    
    CFTimeInterval interval = !throttleTimers ? NullEventIntervalActive : NullEventIntervalNotActive;    
    
    CFRunLoopTimerContext context = { 0, this, NULL, NULL, NULL };
    m_nullEventTimer = adoptCF(CFRunLoopTimerCreate(0, CFAbsoluteTimeGetCurrent() + interval, interval,
                                                   0, 0, nullEventTimerFired, &context));
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), m_nullEventTimer.get(), kCFRunLoopDefaultMode);
}

void WebNetscapePluginEventHandlerCarbon::stopTimers()
{
    if (!m_nullEventTimer)
        return;
    
    CFRunLoopTimerInvalidate(m_nullEventTimer.get());
    m_nullEventTimer = 0;
}

void* WebNetscapePluginEventHandlerCarbon::platformWindow(NSWindow* window)
{
    return [window windowRef];
}

bool WebNetscapePluginEventHandlerCarbon::sendEvent(EventRecord* event)
{
    // If at any point the user clicks or presses a key from within a plugin, set the 
    // currentEventIsUserGesture flag to true. This is important to differentiate legitimate 
    // window.open() calls;  we still want to allow those.  See rdar://problem/4010765
    if (event->what == ::mouseDown || event->what == ::keyDown || event->what == ::mouseUp || event->what == ::autoKey)
        m_currentEventIsUserGesture = true;
    
    m_suspendKeyUpEvents = false; 

    bool result = [m_pluginView sendEvent:event isDrawRect:event->what == updateEvt];
    
    m_currentEventIsUserGesture = false;
    
    return result;
}

#endif // ENABLE(NETSCAPE_PLUGIN_API) && !defined(__LP64__)
