/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#import <JavaScriptCore/JSRetainPtr.h>
#import <WebKit/DOMPrivate.h>
#import <WebKit/WebFramePrivate.h>
#import <WebKit/WebScriptWorld.h>
#import <objc/runtime.h>

namespace TestWebKitAPI {

TEST(WebKit1, DOMNodeFromJSObject)
{
    WebView *webView = [[WebView alloc] initWithFrame:NSZeroRect frameName:nil groupName:nil];

    [webView stringByEvaluatingJavaScriptFromString:@"document.body.mainWorldProperty = true"];

    WebScriptWorld *isolatedWorld = [WebScriptWorld world];
    JSGlobalContextRef context = [[webView mainFrame] _globalContextForScriptWorld:isolatedWorld];

    JSRetainPtr<JSStringRef> script(Adopt, JSStringCreateWithUTF8CString("document.body"));

    JSValueRef value = JSEvaluateScript(context, script.get(), 0, 0, 0, 0);
    JSObjectRef jsBody = JSValueToObject(context, value, 0);

    id objcBody = [DOMNode _nodeFromJSWrapper:jsBody];

    EXPECT_STREQ("DOMHTMLBodyElement", class_getName([objcBody class]));
    EXPECT_EQ([[[webView mainFrame] DOMDocument] body], objcBody);

    // Verify that the Objective-C wrapper is for the main world JS wrapper.
    EXPECT_TRUE([[objcBody valueForKey:@"mainWorldProperty"] boolValue]);

    [webView release];
}

} // namespace TestWebKitAPI
