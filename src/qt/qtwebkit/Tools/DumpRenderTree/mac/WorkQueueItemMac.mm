/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "DumpRenderTree.h"
#import "WorkQueueItem.h"

#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <WebKit/WebBackForwardList.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebScriptObject.h>
#import <WebKit/WebView.h>
#import <wtf/RetainPtr.h>

bool LoadItem::invoke() const
{
    RetainPtr<CFStringRef> urlCF = adoptCF(JSStringCopyCFString(kCFAllocatorDefault, m_url.get()));
    NSString *urlNS = (NSString *)urlCF.get();
    RetainPtr<CFStringRef> targetCF = adoptCF(JSStringCopyCFString(kCFAllocatorDefault, m_target.get()));
    NSString *targetNS = (NSString *)targetCF.get();

    WebFrame *targetFrame;
    if (targetNS && [targetNS length])
        targetFrame = [mainFrame findFrameNamed:targetNS];
    else
        targetFrame = mainFrame;
    [targetFrame loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlNS]]];
    return true;
}

bool LoadHTMLStringItem::invoke() const
{
    RetainPtr<CFStringRef> contentCF = adoptCF(JSStringCopyCFString(kCFAllocatorDefault, m_content.get()));
    RetainPtr<CFStringRef> baseURLCF = adoptCF(JSStringCopyCFString(kCFAllocatorDefault, m_baseURL.get()));

    if (m_unreachableURL) {
        RetainPtr<CFStringRef> unreachableURLCF = adoptCF(JSStringCopyCFString(kCFAllocatorDefault, m_unreachableURL.get()));
        [mainFrame loadAlternateHTMLString:(NSString *)contentCF.get() baseURL:[NSURL URLWithString:(NSString *)baseURLCF.get()] forUnreachableURL:[NSURL URLWithString:(NSString *)unreachableURLCF.get()]];
        return true;
    }

    [mainFrame loadHTMLString:(NSString *)contentCF.get() baseURL:[NSURL URLWithString:(NSString *)baseURLCF.get()]];
    return true;
}

bool ReloadItem::invoke() const
{
    [[mainFrame webView] reload:nil];
    return true;
}

bool ScriptItem::invoke() const
{
    RetainPtr<CFStringRef> scriptCF = adoptCF(JSStringCopyCFString(kCFAllocatorDefault, m_script.get()));
    NSString *scriptNS = (NSString *)scriptCF.get();
    [[mainFrame webView] stringByEvaluatingJavaScriptFromString:scriptNS];
    return true;
}

bool BackForwardItem::invoke() const
{
    if (m_howFar == 1)
        [[mainFrame webView] goForward];
    else if (m_howFar == -1)
        [[mainFrame webView] goBack];
    else {
        WebBackForwardList *bfList = [[mainFrame webView] backForwardList];
        [[mainFrame webView] goToBackForwardItem:[bfList itemAtIndex:m_howFar]];
    }
    return true;
}
