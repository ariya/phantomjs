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
#import "Test.h"

#import "PlatformUtilities.h"
#import "TestBrowsingContextLoadDelegate.h"
#import <JavaScriptCore/JSRetainPtr.h>
#import <JavaScriptCore/JavaScriptCore.h>
#import <WebKit2/WKSerializedScriptValue.h>
#import <WebKit2/WKViewPrivate.h>
#import <WebKit2/WebKit2.h>


static bool testFinished = false;
static NSString *htmlString = @"<body style='background-color: red'>";
static NSString *userStyleSheet = @"body { background-color: green !important; }";
static const char* backgroundColorScript = "window.getComputedStyle(document.body, null).getPropertyValue('background-color')";
static const char* greenInRGB = "rgb(0, 128, 0)";
static const char* redInRGB = "rgb(255, 0, 0)";
static const char* userScriptTestProperty = "window._userScriptInstalled";

namespace {
    class WebKit2UserContentTest : public ::testing::Test {
    public:
        WKProcessGroup *processGroup;
        WKBrowsingContextGroup *browsingContextGroup;

        WebKit2UserContentTest()
            : processGroup(nil)
            , browsingContextGroup(nil)
        {
        }
        
        virtual void SetUp()
        {
            processGroup = [[WKProcessGroup alloc] init];
            browsingContextGroup = [[WKBrowsingContextGroup alloc] initWithIdentifier:@"UserContentIdentifier"];
        }
        
        virtual void TearDown()
        {
            [browsingContextGroup release];
            [processGroup release];
        }
    };
} // namespace

static void expectScriptValueIsString(WKSerializedScriptValueRef serializedScriptValue, const char* expectedValue)
{
    JSGlobalContextRef scriptContext = JSGlobalContextCreate(0);
    
    JSValueRef scriptValue = WKSerializedScriptValueDeserialize(serializedScriptValue, scriptContext, 0);
    EXPECT_TRUE(JSValueIsString(scriptContext, scriptValue));
    
    JSRetainPtr<JSStringRef> scriptString(Adopt, JSValueToStringCopy(scriptContext, scriptValue, 0));
    EXPECT_TRUE(JSStringIsEqualToUTF8CString(scriptString.get(), expectedValue));
    
    JSGlobalContextRelease(scriptContext);
}

static void expectScriptValueIsBoolean(WKSerializedScriptValueRef serializedScriptValue, bool expectedValue)
{
    JSGlobalContextRef scriptContext = JSGlobalContextCreate(0);
    
    JSValueRef scriptValue = WKSerializedScriptValueDeserialize(serializedScriptValue, scriptContext, 0);
    EXPECT_TRUE(JSValueIsBoolean(scriptContext, scriptValue));
    EXPECT_EQ(JSValueToBoolean(scriptContext, scriptValue), expectedValue);
    
    JSGlobalContextRelease(scriptContext);
}

static void expectScriptValueIsUndefined(WKSerializedScriptValueRef serializedScriptValue)
{
    JSGlobalContextRef scriptContext = JSGlobalContextCreate(0);
    
    JSValueRef scriptValue = WKSerializedScriptValueDeserialize(serializedScriptValue, scriptContext, 0);
    EXPECT_TRUE(JSValueIsUndefined(scriptContext, scriptValue));
    
    JSGlobalContextRelease(scriptContext);
}

TEST_F(WebKit2UserContentTest, AddUserStyleSheetBeforeCreatingView)
{
    testFinished = false;
    [browsingContextGroup addUserStyleSheet:userStyleSheet baseURL:nil whitelistedURLPatterns:nil blacklistedURLPatterns:nil mainFrameOnly:YES];
    
    WKView *wkView = [[WKView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) processGroup:processGroup browsingContextGroup:browsingContextGroup];
    WKStringRef backgroundColorQuery = WKStringCreateWithUTF8CString(backgroundColorScript);
    wkView.browsingContextController.loadDelegate = [[TestBrowsingContextLoadDelegate alloc] initWithBlockToRunOnLoad:^(WKBrowsingContextController *sender) {
        WKPageRunJavaScriptInMainFrame_b(wkView.pageRef, backgroundColorQuery, ^(WKSerializedScriptValueRef serializedScriptValue, WKErrorRef error) {
            expectScriptValueIsString(serializedScriptValue, greenInRGB);
            testFinished = true;
            WKRelease(backgroundColorQuery);
        });
    }];
    
    [wkView.browsingContextController loadHTMLString:htmlString baseURL:nil];
    
    TestWebKitAPI::Util::run(&testFinished);
}

TEST_F(WebKit2UserContentTest, AddUserStyleSheetAfterCreatingView)
{
    testFinished = false;
    
    WKView *wkView = [[WKView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) processGroup:processGroup browsingContextGroup:browsingContextGroup];
    WKStringRef backgroundColorQuery = WKStringCreateWithUTF8CString(backgroundColorScript);
    wkView.browsingContextController.loadDelegate = [[TestBrowsingContextLoadDelegate alloc] initWithBlockToRunOnLoad:^(WKBrowsingContextController *sender) {
        WKPageRunJavaScriptInMainFrame_b(wkView.pageRef, backgroundColorQuery, ^(WKSerializedScriptValueRef serializedScriptValue, WKErrorRef error) {
            expectScriptValueIsString(serializedScriptValue, greenInRGB);
            testFinished = true;
            WKRelease(backgroundColorQuery);
        });
    }];
    
    [browsingContextGroup addUserStyleSheet:userStyleSheet baseURL:nil whitelistedURLPatterns:nil blacklistedURLPatterns:nil mainFrameOnly:YES];
    
    [wkView.browsingContextController loadHTMLString:htmlString baseURL:nil];

    TestWebKitAPI::Util::run(&testFinished);
}

TEST_F(WebKit2UserContentTest, RemoveAllUserStyleSheets)
{
    testFinished = false;
    [browsingContextGroup addUserStyleSheet:userStyleSheet baseURL:nil whitelistedURLPatterns:nil blacklistedURLPatterns:nil mainFrameOnly:YES];
    
    WKView *wkView = [[WKView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) processGroup:processGroup browsingContextGroup:browsingContextGroup];
    WKStringRef backgroundColorQuery = WKStringCreateWithUTF8CString(backgroundColorScript);
    wkView.browsingContextController.loadDelegate = [[TestBrowsingContextLoadDelegate alloc] initWithBlockToRunOnLoad:^(WKBrowsingContextController *sender) {
        WKPageRunJavaScriptInMainFrame_b(wkView.pageRef, backgroundColorQuery, ^(WKSerializedScriptValueRef serializedScriptValue, WKErrorRef error) {
            expectScriptValueIsString(serializedScriptValue, redInRGB);
            testFinished = true;
            WKRelease(backgroundColorQuery);
        });
    }];
    
    [browsingContextGroup removeAllUserStyleSheets];
    
    [wkView.browsingContextController loadHTMLString:htmlString baseURL:nil];
    
    TestWebKitAPI::Util::run(&testFinished);
}

TEST_F(WebKit2UserContentTest, AddUserScriptBeforeCreatingView)
{
    testFinished = false;
    [browsingContextGroup addUserScript:[NSString stringWithFormat:@"%s = true;", userScriptTestProperty] baseURL:nil whitelistedURLPatterns:nil blacklistedURLPatterns:nil injectionTime:kWKInjectAtDocumentStart mainFrameOnly:YES];
    
    WKView *wkView = [[WKView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) processGroup:processGroup browsingContextGroup:browsingContextGroup];
    WKStringRef userScriptTestPropertyString = WKStringCreateWithUTF8CString(userScriptTestProperty);
    wkView.browsingContextController.loadDelegate = [[TestBrowsingContextLoadDelegate alloc] initWithBlockToRunOnLoad:^(WKBrowsingContextController *sender) {
        WKPageRunJavaScriptInMainFrame_b(wkView.pageRef, userScriptTestPropertyString, ^(WKSerializedScriptValueRef serializedScriptValue, WKErrorRef error) {
            expectScriptValueIsBoolean(serializedScriptValue, true);
            testFinished = true;
            WKRelease(userScriptTestPropertyString);
        });
    }];
    
    [wkView.browsingContextController loadHTMLString:@"" baseURL:nil];
    
    TestWebKitAPI::Util::run(&testFinished);
}

TEST_F(WebKit2UserContentTest, AddUserScriptAfterCreatingView)
{
    testFinished = false;
    
    WKView *wkView = [[WKView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) processGroup:processGroup browsingContextGroup:browsingContextGroup];
    WKStringRef userScriptTestPropertyString = WKStringCreateWithUTF8CString(userScriptTestProperty);
    wkView.browsingContextController.loadDelegate = [[TestBrowsingContextLoadDelegate alloc] initWithBlockToRunOnLoad:^(WKBrowsingContextController *sender) {
        WKPageRunJavaScriptInMainFrame_b(wkView.pageRef, userScriptTestPropertyString, ^(WKSerializedScriptValueRef serializedScriptValue, WKErrorRef error) {
            expectScriptValueIsBoolean(serializedScriptValue, true);
            testFinished = true;
            WKRelease(userScriptTestPropertyString);
        });
    }];
    
    [browsingContextGroup addUserScript:[NSString stringWithFormat:@"%s = true;", userScriptTestProperty] baseURL:nil whitelistedURLPatterns:nil blacklistedURLPatterns:nil injectionTime:kWKInjectAtDocumentStart mainFrameOnly:YES];
    
    [wkView.browsingContextController loadHTMLString:@"" baseURL:nil];
    
    TestWebKitAPI::Util::run(&testFinished);
}

TEST_F(WebKit2UserContentTest, RemoveAllUserScripts)
{
    testFinished = false;
    [browsingContextGroup addUserScript:[NSString stringWithFormat:@"%s = true;", userScriptTestProperty] baseURL:nil whitelistedURLPatterns:nil blacklistedURLPatterns:nil injectionTime:kWKInjectAtDocumentStart mainFrameOnly:YES];
    
    WKView *wkView = [[WKView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) processGroup:processGroup browsingContextGroup:browsingContextGroup];
    WKStringRef userScriptTestPropertyString = WKStringCreateWithUTF8CString(userScriptTestProperty);
    wkView.browsingContextController.loadDelegate = [[TestBrowsingContextLoadDelegate alloc] initWithBlockToRunOnLoad:^(WKBrowsingContextController *sender) {
        WKPageRunJavaScriptInMainFrame_b(wkView.pageRef, userScriptTestPropertyString, ^(WKSerializedScriptValueRef serializedScriptValue, WKErrorRef error) {
            expectScriptValueIsUndefined(serializedScriptValue);
            testFinished = true;
            WKRelease(userScriptTestPropertyString);
        });
    }];
    
    [browsingContextGroup removeAllUserScripts];
    
    [wkView.browsingContextController loadHTMLString:htmlString baseURL:nil];
    
    TestWebKitAPI::Util::run(&testFinished);
}
