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
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebScriptWorld.h>
#import <JavaScriptCore/JSContextRef.h>
#import <JavaScriptCore/JSRetainPtr.h>
#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSValueRef.h>
#import <wtf/RetainPtr.h>

@interface JSWrapperForNodeFrameLoadDelegate : NSObject {
}
@end

static bool didFinishLoad;

@implementation JSWrapperForNodeFrameLoadDelegate

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, JSWrapperForNode)
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<JSWrapperForNodeFrameLoadDelegate> frameLoadDelegate = adoptNS([[JSWrapperForNodeFrameLoadDelegate alloc] init]);

    webView.get().frameLoadDelegate = frameLoadDelegate.get();
    WebFrame *mainFrame = webView.get().mainFrame;
    [mainFrame loadHTMLString:@"<div id=\"target\"</div>" baseURL:[NSURL URLWithString:@"about:blank"]];
    Util::run(&didFinishLoad);
    DOMDocument *document = webView.get().mainFrameDocument;
    DOMNode *target = [document getElementById:@"target"]; // This script object is in standard world.

    // In an isolated script world, add a new property to the target node.
    NSString *isolatedScriptString = @"var target = document.getElementById(\"target\"); target.isolatedProperty = true;";
    WebScriptWorld *isolatedWorld = [WebScriptWorld world];
    JSGlobalContextRef isolatedCtx = [mainFrame _globalContextForScriptWorld:isolatedWorld];
    [mainFrame _stringByEvaluatingJavaScriptFromString:isolatedScriptString withGlobalObject:JSContextGetGlobalObject(isolatedCtx) inScriptWorld:isolatedWorld];
    JSValueRef isolatedNodeJSValue = [mainFrame jsWrapperForNode:target inScriptWorld:isolatedWorld];
    ASSERT_TRUE(JSValueIsObject(isolatedCtx, isolatedNodeJSValue));
    JSObjectRef isolatedNodeJSObject = JSValueToObject(isolatedCtx, isolatedNodeJSValue, 0);

    // In the standard script world, add a different property to the target node
    NSString *normalScriptString = @"var target = document.getElementById(\"target\"); target.normalProperty = true;";
    WebScriptWorld *normalWorld = [WebScriptWorld standardWorld];
    JSGlobalContextRef normalCtx = [mainFrame _globalContextForScriptWorld:normalWorld];
    [mainFrame _stringByEvaluatingJavaScriptFromString:normalScriptString withGlobalObject:JSContextGetGlobalObject(normalCtx) inScriptWorld:normalWorld];
    JSValueRef normalNodeJSValue = [mainFrame jsWrapperForNode:target inScriptWorld:normalWorld];
    ASSERT_TRUE(JSValueIsObject(normalCtx, normalNodeJSValue));
    JSObjectRef normalNodeJSObject = JSValueToObject(normalCtx, normalNodeJSValue, 0);

    JSRetainPtr<JSStringRef> isolatedPropertyJSString = JSStringCreateWithUTF8CString("isolatedProperty");
    // Test for successful retrieval of the first property in the isolated script world
    EXPECT_TRUE(JSValueIsBoolean(isolatedCtx, JSObjectGetProperty(isolatedCtx, isolatedNodeJSObject, isolatedPropertyJSString.get(), 0)));
    // Test for failed retrieval of the first property in the standard script world
    EXPECT_TRUE(JSValueIsUndefined(normalCtx, JSObjectGetProperty(normalCtx, normalNodeJSObject, isolatedPropertyJSString.get(), 0)));

    JSRetainPtr<JSStringRef> normalPropertyJSString = JSStringCreateWithUTF8CString("normalProperty");
    // Test for successful retrieval of the second property in the standard script world
    EXPECT_TRUE(JSValueIsBoolean(normalCtx, JSObjectGetProperty(normalCtx, normalNodeJSObject, normalPropertyJSString.get(), 0)));
    // Test for failed retrieval of the second property in the isolated script world
    EXPECT_TRUE(JSValueIsUndefined(isolatedCtx, JSObjectGetProperty(isolatedCtx, isolatedNodeJSObject, normalPropertyJSString.get(), 0)));
}

} // namespace TestWebKitAPI
