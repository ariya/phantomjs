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

#ifndef WebNetscapePluginEventHandlerCarbon_h
#define WebNetscapePluginEventHandlerCarbon_h

#include "WebNetscapePluginEventHandler.h"

#import <Carbon/Carbon.h>
#import <wtf/RetainPtr.h>

class WebNetscapePluginEventHandlerCarbon : public WebNetscapePluginEventHandler {
public:
    WebNetscapePluginEventHandlerCarbon(WebNetscapePluginView*); 

    virtual void drawRect(CGContextRef, const NSRect&);

    virtual void mouseDown(NSEvent*);
    virtual void mouseDragged(NSEvent*);
    virtual void mouseEntered(NSEvent*);
    virtual void mouseExited(NSEvent*);
    virtual void mouseMoved(NSEvent*);
    virtual void mouseUp(NSEvent*);
    virtual bool scrollWheel(NSEvent*);
    
    virtual void keyDown(NSEvent*);
    virtual void keyUp(NSEvent*);
    virtual void flagsChanged(NSEvent*);
    virtual void syntheticKeyDownWithCommandModifier(int keyCode, char character);

    virtual void windowFocusChanged(bool hasFocus);    
    virtual void focusChanged(bool hasFocus);

    virtual void startTimers(bool throttleTimers);
    virtual void stopTimers();

    virtual void* platformWindow(NSWindow*);
    
private:
    void sendNullEvent();

    void installKeyEventHandler();
    void removeKeyEventHandler();
    
    static OSStatus TSMEventHandler(EventHandlerCallRef, EventRef, void *eventHandler);
    static void nullEventTimerFired(CFRunLoopTimerRef, void *context);

    bool sendEvent(EventRecord*);
    
    EventHandlerRef m_keyEventHandler;
    bool m_suspendKeyUpEvents;
    RetainPtr<CFRunLoopTimerRef> m_nullEventTimer;
};

#endif // WebNetscapePluginEventHandlerCarbon_h

#endif // ENABLE(NETSCAPE_PLUGIN_API) && !defined(__LP64__)

