/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#ifndef AccessibilityController_h
#define AccessibilityController_h

#include "AccessibilityUIElement.h"
#include "JSWrappable.h"
#include <JavaScriptCore/JSObjectRef.h>
#include <wtf/Platform.h>
#if PLATFORM(WIN)
#include <windows.h>
#endif

namespace WTR {
    
class AccessibilityController : public JSWrappable {
public:
    static PassRefPtr<AccessibilityController> create();
    ~AccessibilityController();

    void makeWindowObject(JSContextRef, JSObjectRef windowObject, JSValueRef* exception);
    virtual JSClassRef wrapperClass();
    
    // Controller Methods - platform-independent implementations.
    PassRefPtr<AccessibilityUIElement> rootElement();
    PassRefPtr<AccessibilityUIElement> focusedElement();
    PassRefPtr<AccessibilityUIElement> elementAtPoint(int x, int y);
    PassRefPtr<AccessibilityUIElement> accessibleElementById(JSStringRef idAttribute);

    bool addNotificationListener(JSValueRef functionCallback);
    bool removeNotificationListener();

    // Here for consistency with DRT. Not implemented because they don't do anything on the Mac.
    void logFocusEvents() { }
    void logValueChangeEvents() { }
    void logScrollingStartEvents() { }
    void logAccessibilityEvents();

    void resetToConsistentState();

private:
    AccessibilityController();

#if PLATFORM(MAC)
    RetainPtr<NotificationHandler> m_globalNotificationHandler;
#endif

#if PLATFORM(GTK) || PLATFORM(EFL)
    unsigned m_stateChangeListenerId;
    unsigned m_focusEventListenerId;
    unsigned m_activeDescendantChangedListenerId;
    unsigned m_childrenChangedListenerId;
    unsigned m_propertyChangedListenerId;
    unsigned m_visibleDataChangedListenerId;
#endif
};

} // namespace WTR

#endif // AccessibilityController_h
