/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "AccessibilityCommonMac.h"
#import "AccessibilityController.h"
#import "AccessibilityNotificationHandler.h"
#import "InjectedBundle.h"
#import "InjectedBundlePage.h"

#import <JavaScriptCore/JSRetainPtr.h>
#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <WebKit2/WKBundle.h>
#import <WebKit2/WKBundlePage.h>
#import <WebKit2/WKBundlePagePrivate.h>

namespace WTR {

bool AccessibilityController::addNotificationListener(JSValueRef functionCallback)
{
    if (!functionCallback)
        return false;

    if (m_globalNotificationHandler)
        return false;
    m_globalNotificationHandler = [[AccessibilityNotificationHandler alloc] init];
    [m_globalNotificationHandler.get() setCallback:functionCallback];
    [m_globalNotificationHandler.get() startObserving];

    return true;
}

bool AccessibilityController::removeNotificationListener()
{
    ASSERT(m_globalNotificationHandler);
    
    [m_globalNotificationHandler.get() stopObserving];
    m_globalNotificationHandler.clear();

    return true;
}

void AccessibilityController::logAccessibilityEvents()
{
}

void AccessibilityController::resetToConsistentState()
{
    if (m_globalNotificationHandler)
        removeNotificationListener();
}

static id findAccessibleObjectById(id obj, NSString *idAttribute)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id objIdAttribute = [obj accessibilityAttributeValue:@"AXDRTElementIdAttribute"];
    if ([objIdAttribute isKindOfClass:[NSString class]] && [objIdAttribute isEqualToString:idAttribute])
        return obj;
    END_AX_OBJC_EXCEPTIONS

    NSArray *children = [obj accessibilityAttributeValue:NSAccessibilityChildrenAttribute];
    NSUInteger childrenCount = [children count];
    for (NSUInteger i = 0; i < childrenCount; ++i) {
        id result = findAccessibleObjectById([children objectAtIndex:i], idAttribute);
        if (result)
            return result;
    }

    return 0;
}

PassRefPtr<AccessibilityUIElement> AccessibilityController::accessibleElementById(JSStringRef idAttribute)
{
    WKBundlePageRef page = InjectedBundle::shared().page()->page();
    id root = static_cast<PlatformUIElement>(WKAccessibilityRootObject(page));

    id result = findAccessibleObjectById(root, [NSString stringWithJSStringRef:idAttribute]);
    if (result)
        return AccessibilityUIElement::create(result);

    return 0;
}

} // namespace WTR
