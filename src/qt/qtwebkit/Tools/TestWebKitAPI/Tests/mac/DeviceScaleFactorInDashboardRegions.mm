/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#import "PlatformUtilities.h"
#import "SyntheticBackingScaleFactorWindow.h"
#import "Test.h"
#import <WebKit/WebDashboardRegion.h>
#import <wtf/RetainPtr.h>

static bool gotDashboardRegions;
static NSDictionary *regions;

@interface DeviceScaleFactorInDashboardRegionsUIDelegate : NSObject {
}
@end

@implementation DeviceScaleFactorInDashboardRegionsUIDelegate

- (void)webView:(WebView *)webView dashboardRegionsChanged:(NSDictionary *)newRegions
{
    gotDashboardRegions = true;
    regions = [newRegions retain];
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, DeviceScaleFactorInDashboardRegions)
{
    NSRect viewFrame = NSMakeRect(0, 0, 800, 600);
    RetainPtr<SyntheticBackingScaleFactorWindow> window = adoptNS([[SyntheticBackingScaleFactorWindow alloc] initWithContentRect:viewFrame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES]);
    [window.get() setReleasedWhenClosed:NO];
    [window.get() setBackingScaleFactor:2];

    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:viewFrame frameName:nil groupName:nil]);
    RetainPtr<DeviceScaleFactorInDashboardRegionsUIDelegate> uiDelegate = adoptNS([DeviceScaleFactorInDashboardRegionsUIDelegate new]);
    webView.get().UIDelegate = uiDelegate.get();
    [window.get().contentView addSubview:webView.get()];

    [webView.get().mainFrame loadHTMLString:@"<div style='position: absolute; top: 10px; left: 10px; width: 50px; height: 50px; -webkit-dashboard-region: dashboard-region(control rectangle);'></div>" baseURL:[NSURL URLWithString:@"about:blank"]];

    Util::run(&gotDashboardRegions);

    NSRect controlRegionRect = [[[regions objectForKey:@"control"] objectAtIndex:0] dashboardRegionRect];

    EXPECT_EQ(10, controlRegionRect.origin.x);
    EXPECT_EQ(10, controlRegionRect.origin.y);
    EXPECT_EQ(50, controlRegionRect.size.width);
    EXPECT_EQ(50, controlRegionRect.size.height);

    [regions release];
}

} // namespace TestWebKitAPI
