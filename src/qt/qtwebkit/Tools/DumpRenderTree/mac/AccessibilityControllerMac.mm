/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
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

#import "config.h"
#import "DumpRenderTree.h"
#import "AccessibilityController.h"

#import "AccessibilityCommonMac.h"
#import "AccessibilityNotificationHandler.h"
#import "AccessibilityUIElement.h"
#import <AppKit/NSColor.h>
#import <Foundation/Foundation.h>
#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebHTMLView.h>

AccessibilityController::AccessibilityController()
{
}

AccessibilityController::~AccessibilityController()
{
    // The notification handler should be nil because removeNotificationListener() should have been called in the test.
    ASSERT(!m_globalNotificationHandler);
}

AccessibilityUIElement AccessibilityController::elementAtPoint(int x, int y)
{
    id accessibilityObject = [[[mainFrame frameView] documentView] accessibilityHitTest:NSMakePoint(x, y)];
    return AccessibilityUIElement(accessibilityObject);    
}

AccessibilityUIElement AccessibilityController::focusedElement()
{
    id accessibilityObject = [[mainFrame accessibilityRoot] accessibilityFocusedUIElement];
    return AccessibilityUIElement(accessibilityObject);
}

AccessibilityUIElement AccessibilityController::rootElement()
{
    // FIXME: we could do some caching here.
    
    // Layout tests expect that the root element will be the scroll area
    // containing the web area object. That will be the parent of the accessibilityRoot on WK1.
    
    id accessibilityObject = [[mainFrame accessibilityRoot] accessibilityAttributeValue:NSAccessibilityParentAttribute];
    return AccessibilityUIElement(accessibilityObject);
}

static id findAccessibleObjectById(id obj, NSString *idAttribute)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id objIdAttribute = [obj accessibilityAttributeValue:@"AXDRTElementIdAttribute"];
    if ([objIdAttribute isKindOfClass:[NSString class]] && [objIdAttribute isEqualToString:idAttribute])
        return obj;
    END_AX_OBJC_EXCEPTIONS

    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray *children = [obj accessibilityAttributeValue:NSAccessibilityChildrenAttribute];
    NSUInteger childrenCount = [children count];
    for (NSUInteger i = 0; i < childrenCount; ++i) {
        id result = findAccessibleObjectById([children objectAtIndex:i], idAttribute);
        if (result)
            return result;
    }
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

AccessibilityUIElement AccessibilityController::accessibleElementById(JSStringRef idAttributeRef)
{
    NSString *idAttribute = [NSString stringWithJSStringRef:idAttributeRef];
    id root = [[mainFrame accessibilityRoot] accessibilityAttributeValue:NSAccessibilityParentAttribute];
    id result = findAccessibleObjectById(root, idAttribute);
    if (result)
        return AccessibilityUIElement(result);

    return 0;
}

void AccessibilityController::setLogFocusEvents(bool)
{
}

void AccessibilityController::setLogScrollingStartEvents(bool)
{
}

void AccessibilityController::setLogValueChangeEvents(bool)
{
}

void AccessibilityController::setLogAccessibilityEvents(bool)
{
}

bool AccessibilityController::addNotificationListener(JSObjectRef functionCallback)
{
    if (!functionCallback)
        return false;
 
    // Mac programmers should not be adding more than one global notification listener.
    // Other platforms may be different.
    if (m_globalNotificationHandler)
        return false;
    m_globalNotificationHandler = [[AccessibilityNotificationHandler alloc] init];
    [m_globalNotificationHandler.get() setCallback:functionCallback];
    [m_globalNotificationHandler.get() startObserving];

    return true;
}

void AccessibilityController::removeNotificationListener()
{
    // Mac programmers should not be trying to remove a listener that's already removed.
    ASSERT(m_globalNotificationHandler);
    m_globalNotificationHandler.clear();
}
