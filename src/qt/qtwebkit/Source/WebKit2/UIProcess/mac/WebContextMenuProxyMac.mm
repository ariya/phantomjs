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
#import "WebContextMenuProxyMac.h"

#import "PageClientImpl.h"
#import "StringUtilities.h"
#import "WebContextMenuItemData.h"
#import "WKView.h"

#import <WebCore/IntRect.h>
#import <WebKitSystemInterface.h>

using namespace WebCore;

@interface WKUserDataWrapper : NSObject {
    RefPtr<WebKit::APIObject> _webUserData;
}
- (id)initWithUserData:(WebKit::APIObject*)userData;
- (WebKit::APIObject*)userData;
@end

@implementation WKUserDataWrapper

- (id)initWithUserData:(WebKit::APIObject*)userData
{
    self = [super init];
    if (!self)
        return nil;
    
    _webUserData = userData;
    return self;
}

- (WebKit::APIObject*)userData
{
    return _webUserData.get();
}

@end

@interface WKMenuTarget : NSObject {
    WebKit::WebContextMenuProxyMac* _menuProxy;
}
+ (WKMenuTarget*)sharedMenuTarget;
- (WebKit::WebContextMenuProxyMac*)menuProxy;
- (void)setMenuProxy:(WebKit::WebContextMenuProxyMac*)menuProxy;
- (void)forwardContextMenuAction:(id)sender;
@end

@implementation WKMenuTarget

+ (WKMenuTarget*)sharedMenuTarget
{
    static WKMenuTarget* target = [[WKMenuTarget alloc] init];
    return target;
}

- (WebKit::WebContextMenuProxyMac*)menuProxy
{
    return _menuProxy;
}

- (void)setMenuProxy:(WebKit::WebContextMenuProxyMac*)menuProxy
{
    _menuProxy = menuProxy;
}

- (void)forwardContextMenuAction:(id)sender
{
    WebKit::WebContextMenuItemData item(ActionType, static_cast<ContextMenuAction>([sender tag]), [sender title], [sender isEnabled], [sender state] == NSOnState);
    
    if (id representedObject = [sender representedObject]) {
        ASSERT([representedObject isKindOfClass:[WKUserDataWrapper class]]);
        item.setUserData([static_cast<WKUserDataWrapper *>(representedObject) userData]);
    }
            
    _menuProxy->contextMenuItemSelected(item);
}

@end

namespace WebKit {

WebContextMenuProxyMac::WebContextMenuProxyMac(WKView* webView, WebPageProxy* page)
    : m_webView(webView)
    , m_page(page)
{
}

WebContextMenuProxyMac::~WebContextMenuProxyMac()
{
    if (m_popup)
        [m_popup.get() setControlView:nil];
}

void WebContextMenuProxyMac::contextMenuItemSelected(const WebContextMenuItemData& item)
{
    m_page->contextMenuItemSelected(item);
}

static void populateNSMenu(NSMenu* menu, const Vector<RetainPtr<NSMenuItem>>& menuItemVector)
{
    for (unsigned i = 0; i < menuItemVector.size(); ++i) {
        NSInteger oldState = [menuItemVector[i].get() state];
        [menu addItem:menuItemVector[i].get()];
        [menuItemVector[i].get() setState:oldState];
    }
}

static Vector<RetainPtr<NSMenuItem>> nsMenuItemVector(const Vector<WebContextMenuItemData>& items)
{
    Vector<RetainPtr<NSMenuItem>> result;

    unsigned size = items.size();
    result.reserveCapacity(size);
    for (unsigned i = 0; i < size; i++) {
        switch (items[i].type()) {
        case ActionType:
        case CheckableActionType: {
            NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:nsStringFromWebCoreString(items[i].title()) action:@selector(forwardContextMenuAction:) keyEquivalent:@""];
            [menuItem setTag:items[i].action()];
            [menuItem setEnabled:items[i].enabled()];
            [menuItem setState:items[i].checked() ? NSOnState : NSOffState];
                        
            if (items[i].userData()) {
                WKUserDataWrapper *wrapper = [[WKUserDataWrapper alloc] initWithUserData:items[i].userData()];
                [menuItem setRepresentedObject:wrapper];
                [wrapper release];
            }

            result.append(adoptNS(menuItem));
            break;
        }
        case SeparatorType:
            result.append([NSMenuItem separatorItem]);
            break;
        case SubmenuType: {
            NSMenu* menu = [[NSMenu alloc] initWithTitle:nsStringFromWebCoreString(items[i].title())];
            [menu setAutoenablesItems:NO];
            populateNSMenu(menu, nsMenuItemVector(items[i].submenu()));
                
            NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:nsStringFromWebCoreString(items[i].title()) action:@selector(forwardContextMenuAction:) keyEquivalent:@""];
            [menuItem setEnabled:items[i].enabled()];
            [menuItem setSubmenu:menu];
            [menu release];

            result.append(adoptNS(menuItem));
            
            break;
        }
        default:
            ASSERT_NOT_REACHED();
        }
    }

    WKMenuTarget* target = [WKMenuTarget sharedMenuTarget];
    for (unsigned i = 0; i < size; ++i)
        [result[i].get() setTarget:target];
    
    return result;
}

void WebContextMenuProxyMac::populate(const Vector<WebContextMenuItemData>& items)
{
    if (m_popup)
        [m_popup.get() removeAllItems];
    else {
        m_popup = adoptNS([[NSPopUpButtonCell alloc] initTextCell:@"" pullsDown:NO]);
        [m_popup.get() setUsesItemFromMenu:NO];
        [m_popup.get() setAutoenablesItems:NO];
    }

    NSMenu* menu = [m_popup.get() menu];
    populateNSMenu(menu, nsMenuItemVector(items));
}

void WebContextMenuProxyMac::showContextMenu(const IntPoint& menuLocation, const Vector<WebContextMenuItemData>& items)
{
    if (items.isEmpty())
        return;
    
    populate(items);
    [[WKMenuTarget sharedMenuTarget] setMenuProxy:this];
    
    NSRect menuRect = NSMakeRect(menuLocation.x(), menuLocation.y(), 0, 0);
    
    [m_popup.get() attachPopUpWithFrame:menuRect inView:m_webView];

    NSMenu* menu = [m_popup.get() menu];

    // These values were borrowed from AppKit to match their placement of the menu.
    NSRect titleFrame = [m_popup.get()  titleRectForBounds:menuRect];
    if (titleFrame.size.width <= 0 || titleFrame.size.height <= 0)
        titleFrame = menuRect;
    float vertOffset = roundf((NSMaxY(menuRect) - NSMaxY(titleFrame)) + NSHeight(titleFrame));
    NSPoint location = NSMakePoint(NSMinX(menuRect), NSMaxY(menuRect) - vertOffset);

    location = [m_webView convertPoint:location toView:nil];
    location = [m_webView.window convertBaseToScreen:location];
 
    WKPopupContextMenu(menu, location);

    [m_popup.get() dismissPopUp];
}

void WebContextMenuProxyMac::hideContextMenu()
{
    [m_popup.get() dismissPopUp];
}

} // namespace WebKit
