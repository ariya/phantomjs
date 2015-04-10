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

#include <Cocoa/Cocoa.h>
#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKBundleInitialize.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKStringCF.h>
#include <WebKit2/WKURLCF.h>
#include <stdio.h>

static WKBundleRef globalBundle;

// WKBundlePageClient functions

void didClearWindowObjectForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void *clientInfo)
{
    WKURLRef wkURL = WKBundleFrameCopyURL(WKBundlePageGetMainFrame(page));
    CFURLRef cfURL = WKURLCopyCFURL(0, wkURL);
    WKRelease(wkURL);

    LOG(@"WKBundlePageClient - didClearWindowForFrame %@", [(NSURL *)cfURL absoluteString]);
    if (cfURL)
        CFRelease(cfURL);

    WKStringRef messageName = WKStringCreateWithCFString(CFSTR("Callback"));
    WKStringRef messageBody = WKStringCreateWithCFString(CFSTR("Window was cleared"));
    WKBundlePostMessage(globalBundle, messageName, messageBody);
    WKRelease(messageName);
    WKRelease(messageBody);
}


// WKBundleClient

void didCreatePage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
    LOG(@"WKBundleClient - didCreatePage\n");

    WKBundlePageLoaderClient client;
    memset(&client, 0, sizeof(client));
    client.didClearWindowObjectForFrame = didClearWindowObjectForFrame;

    WKBundlePageSetPageLoaderClient(page, &client);
}

void willDestroyPage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
    LOG(@"WKBundleClient - willDestroyPage\n");
}

void didReceiveMessage(WKBundleRef bundle, WKStringRef messageName, WKTypeRef messageBody, const void *clientInfo)
{
    CFStringRef cfMessageName = WKStringCopyCFString(0, messageName);

    WKTypeID typeID = WKGetTypeID(messageBody);
    if (typeID == WKStringGetTypeID()) {
        CFStringRef cfMessageBody = WKStringCopyCFString(0, (WKStringRef)messageBody);
        LOG(@"WKBundleClient - didReceiveMessage - MessageName: %@ MessageBody %@", cfMessageName, cfMessageBody);
        CFRelease(cfMessageBody);
    } else {
        LOG(@"WKBundleClient - didReceiveMessage - MessageName: %@ (MessageBody Unhandled)\n", cfMessageName);
    }

    CFRelease(cfMessageName);
}

void WKBundleInitialize(WKBundleRef bundle, WKTypeRef initializationUserData)
{
    globalBundle = bundle;

    WKBundleClient client = {
        kWKBundleClientCurrentVersion,
        0,
        didCreatePage,
        willDestroyPage,
        0, // didInitializePageGroup
        didReceiveMessage,
        0 // didReceiveMessageToPage
    };
    WKBundleSetClient(bundle, &client);
}
