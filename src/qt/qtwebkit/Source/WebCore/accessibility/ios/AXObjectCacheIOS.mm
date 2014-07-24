/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "AXObjectCache.h"

#if HAVE(ACCESSIBILITY) && PLATFORM(IOS)

#import "AccessibilityObject.h"
#import "WebAccessibilityObjectWrapperIOS.h"
#import "RenderObject.h"

#import <wtf/PassRefPtr.h>
#import <wtf/RetainPtr.h>

namespace WebCore {
    
void AXObjectCache::detachWrapper(AccessibilityObject* obj)
{
    [obj->wrapper() detach];
    obj->setWrapper(0);
}

void AXObjectCache::attachWrapper(AccessibilityObject* obj)
{
    RetainPtr<AccessibilityObjectWrapper> wrapper = adoptNS([[WebAccessibilityObjectWrapper alloc] initWithAccessibilityObject:obj]);
    obj->setWrapper(wrapper.get());
}

void AXObjectCache::postPlatformNotification(AccessibilityObject* obj, AXNotification notification)
{
    if (!obj)
        return;

    NSString *notificationString = nil;
    switch (notification) {
        case AXActiveDescendantChanged:
        case AXFocusedUIElementChanged:
            [obj->wrapper() postFocusChangeNotification];
            notificationString = @"AXFocusChanged";
            break;
        case AXSelectedTextChanged:
            [obj->wrapper() postSelectedTextChangeNotification];
            break;
        case AXLayoutComplete:
            [obj->wrapper() postLayoutChangeNotification];
            break;
        case AXLiveRegionChanged:
            [obj->wrapper() postLiveRegionChangeNotification];
            break;
        case AXChildrenChanged:
            [obj->wrapper() postChildrenChangedNotification];
            break;
        case AXLoadComplete:
            [obj->wrapper() postLoadCompleteNotification];
            break;
        case AXInvalidStatusChanged:
            [obj->wrapper() postInvalidStatusChangedNotification];
            break;
        case AXSelectedChildrenChanged:
        case AXValueChanged:
        case AXCheckedStateChanged:
        default:
            break;
    }
    
    // Used by DRT to know when notifications are posted.
    [obj->wrapper() accessibilityPostedNotification:notificationString];
}

void AXObjectCache::nodeTextChangePlatformNotification(AccessibilityObject*, AXTextChange, unsigned, const String&)
{
}

void AXObjectCache::frameLoadingEventPlatformNotification(AccessibilityObject*, AXLoadingEvent)
{
}

void AXObjectCache::handleFocusedUIElementChanged(Node*, Node* newNode)
{
    postNotification(newNode, AXFocusedUIElementChanged, true, PostAsynchronously);
}

void AXObjectCache::handleScrolledToAnchor(const Node*)
{
}
    
}

#endif // HAVE(ACCESSIBILITY) && PLATFORM(IOS)
