/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "WKAccessibilityWebPageObject.h"

#import "WebFrame.h"
#import "WebPage.h"
#import <WebCore/AXObjectCache.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/Page.h>
#import <WebCore/ScrollView.h>
#import <WebCore/Scrollbar.h>
#import <WebKitSystemInterface.h>

using namespace WebCore;
using namespace WebKit;

@implementation WKAccessibilityWebPageObject

- (id)accessibilityRootObjectWrapper
{
    if (!WebCore::AXObjectCache::accessibilityEnabled())
        WebCore::AXObjectCache::enableAccessibility();

    NSObject* mainFramePluginAccessibilityObjectWrapper = m_page->accessibilityObjectForMainFramePlugin();
    if (mainFramePluginAccessibilityObjectWrapper)
        return mainFramePluginAccessibilityObjectWrapper;

    WebCore::Page* page = m_page->corePage();
    if (!page)
        return nil;
    
    WebCore::Frame* core = page->mainFrame();
    if (!core || !core->document())
        return nil;
    
    AccessibilityObject* root = core->document()->axObjectCache()->rootObject();
    if (!root)
        return nil;
    
    return root->wrapper();
}

- (void)setWebPage:(WebPage*)page
{
    m_page = page;
}

- (void)setRemoteParent:(id)parent
{
    if (parent != m_parent) {
        [m_parent release];
        m_parent = [parent retain];
    }
}

- (void)dealloc
{
    WKUnregisterUniqueIdForElement(self);
    [m_accessibilityChildren release];
    [m_attributeNames release];
    [m_parent release];
    [super dealloc];
}

- (BOOL)accessibilityIsIgnored
{
    return NO;
}

- (NSArray *)accessibilityAttributeNames
{
    if (!m_attributeNames)
        m_attributeNames = [[NSArray alloc] initWithObjects:
                           NSAccessibilityRoleAttribute, NSAccessibilityRoleDescriptionAttribute, NSAccessibilityFocusedAttribute,
                           NSAccessibilityParentAttribute, NSAccessibilityWindowAttribute, NSAccessibilityTopLevelUIElementAttribute,
                           NSAccessibilityPositionAttribute, NSAccessibilitySizeAttribute, NSAccessibilityChildrenAttribute, nil];

    return m_attributeNames;
}

- (BOOL)accessibilityIsAttributeSettable:(NSString *)attribute
{
    return NO;
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute 
{
    return;
}

- (NSArray *)accessibilityActionNames
{
    return [NSArray array];
}

- (NSArray *)accessibilityChildren
{
    id wrapper = [self accessibilityRootObjectWrapper];
    if (!wrapper)
        return [NSArray array];

    return [NSArray arrayWithObject:wrapper];
}

- (id)accessibilityAttributeValue:(NSString *)attribute
{
    if (!WebCore::AXObjectCache::accessibilityEnabled())
        WebCore::AXObjectCache::enableAccessibility();
    
    if ([attribute isEqualToString:NSAccessibilityParentAttribute])
        return m_parent;
    if ([attribute isEqualToString:NSAccessibilityWindowAttribute])
        return [m_parent accessibilityAttributeValue:NSAccessibilityWindowAttribute];
    if ([attribute isEqualToString:NSAccessibilityTopLevelUIElementAttribute])
        return [m_parent accessibilityAttributeValue:NSAccessibilityTopLevelUIElementAttribute];
    if ([attribute isEqualToString:NSAccessibilityRoleAttribute])
        return NSAccessibilityGroupRole;
    if ([attribute isEqualToString:NSAccessibilityRoleDescriptionAttribute])
        return NSAccessibilityRoleDescription(NSAccessibilityGroupRole, nil);
    if ([attribute isEqualToString:NSAccessibilityFocusedAttribute])
        return [NSNumber numberWithBool:NO];

    if (!m_page)
        return nil;

    if ([attribute isEqualToString:NSAccessibilityPositionAttribute]) {
        const WebCore::FloatPoint& point = m_page->accessibilityPosition();
        return [NSValue valueWithPoint:NSMakePoint(point.x(), point.y())];
    }
    if ([attribute isEqualToString:NSAccessibilitySizeAttribute]) {
        const IntSize& s = m_page->size();
        return [NSValue valueWithSize:NSMakeSize(s.width(), s.height())];
    }
    if ([attribute isEqualToString:NSAccessibilityChildrenAttribute])
        return [self accessibilityChildren];

    return nil;
}

- (BOOL)accessibilityShouldUseUniqueId
{
    return YES;
}

- (id)accessibilityHitTest:(NSPoint)point 
{
    // Hit-test point comes in as bottom-screen coordinates. Needs to be normalized to the frame of the web page.
    NSPoint remotePosition = [[self accessibilityAttributeValue:NSAccessibilityPositionAttribute] pointValue];
    NSSize remoteSize = [[self accessibilityAttributeValue:NSAccessibilitySizeAttribute] sizeValue];
    
    // Get the y position of the WKView (we have to screen-flip and go from bottom left to top left).
    CGFloat screenHeight = [(NSScreen *)[[NSScreen screens] objectAtIndex:0] frame].size.height;
    remotePosition.y = (screenHeight - remotePosition.y) - remoteSize.height;
    
    point.y = screenHeight - point.y;

    // Re-center point into the web page's frame.
    point.y -= remotePosition.y;
    point.x -= remotePosition.x;
    
    WebCore::FrameView* frameView = m_page ? m_page->mainFrameView() : 0;
    if (frameView) {
        point.y += frameView->scrollPosition().y();
        point.x += frameView->scrollPosition().x();
    }
    
    return [[self accessibilityRootObjectWrapper] accessibilityHitTest:point];
}

- (id)accessibilityFocusedUIElement 
{
    return [[self accessibilityRootObjectWrapper] accessibilityFocusedUIElement];
}


@end
