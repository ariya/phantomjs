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
#import "InjectedBundle.h"

#import "ObjCObjectGraph.h"
#import "WKBundleAPICast.h"
#import "WKBundleInitialize.h"
#import "WKWebProcessPlugInInternal.h"
#import <Foundation/NSBundle.h>
#import <stdio.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/CString.h>
#import <wtf/text/WTFString.h>

using namespace WebCore;

@interface NSBundle (WKAppDetails)
- (CFBundleRef)_cfBundle;
@end

namespace WebKit {

bool InjectedBundle::load(APIObject* initializationUserData)
{
    if (m_sandboxExtension) {
        if (!m_sandboxExtension->consumePermanently()) {
            WTFLogAlways("InjectedBundle::load failed - Could not consume bundle sandbox extension for [%s].\n", m_path.utf8().data());
            return false;
        }

        m_sandboxExtension = 0;
    }
    
    RetainPtr<CFStringRef> injectedBundlePathStr = adoptCF(CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar*>(m_path.characters()), m_path.length()));
    if (!injectedBundlePathStr) {
        WTFLogAlways("InjectedBundle::load failed - Could not create the path string.\n");
        return false;
    }
    
    RetainPtr<CFURLRef> bundleURL = adoptCF(CFURLCreateWithFileSystemPath(0, injectedBundlePathStr.get(), kCFURLPOSIXPathStyle, false));
    if (!bundleURL) {
        WTFLogAlways("InjectedBundle::load failed - Could not create the url from the path string.\n");
        return false;
    }

    m_platformBundle = [[NSBundle alloc] initWithURL:(NSURL *)bundleURL.get()];
    if (!m_platformBundle) {
        WTFLogAlways("InjectedBundle::load failed - Could not create the bundle.\n");
        return false;
    }
        
    if (![m_platformBundle load]) {
        WTFLogAlways("InjectedBundle::load failed - Could not load the executable from the bundle.\n");
        return false;
    }

    // First check to see if the bundle has a WKBundleInitialize function.
    WKBundleInitializeFunctionPtr initializeFunction = reinterpret_cast<WKBundleInitializeFunctionPtr>(CFBundleGetFunctionPointerForName([m_platformBundle _cfBundle], CFSTR("WKBundleInitialize")));
    if (initializeFunction) {
        initializeFunction(toAPI(this), toAPI(initializationUserData));
        return true;
    }
    
#if defined(__LP64__) && defined(__clang__)
    // Otherwise, look to see if the bundle has a principal class
    Class principalClass = [m_platformBundle principalClass];
    if (!principalClass) {
        WTFLogAlways("InjectedBundle::load failed - No initialize function or principal class found in the bundle executable.\n");
        return false;
    }

    if (![principalClass conformsToProtocol:@protocol(WKWebProcessPlugIn)]) {
        WTFLogAlways("InjectedBundle::load failed - Principal class does not conform to the WKWebProcessPlugIn protocol.\n");
        return false;
    }

    id<WKWebProcessPlugIn> instance = (id<WKWebProcessPlugIn>)[[principalClass alloc] init];
    if (!instance) {
        WTFLogAlways("InjectedBundle::load failed - Could not initialize an instance of the principal class.\n");
        return false;
    }

    // Create the shared WKWebProcessPlugInController.
    [[WKWebProcessPlugInController alloc] _initWithPrincipalClassInstance:instance bundleRef:toAPI(this)];

    if ([instance respondsToSelector:@selector(webProcessPlugIn:initializeWithObject:)]) {
        RetainPtr<id> objCInitializationUserData;
        if (initializationUserData && initializationUserData->type() == APIObject::TypeObjCObjectGraph)
            objCInitializationUserData = static_cast<ObjCObjectGraph*>(initializationUserData)->rootObject();
        [instance webProcessPlugIn:[WKWebProcessPlugInController _shared] initializeWithObject:objCInitializationUserData.get()];
    } else if ([instance respondsToSelector:@selector(webProcessPlugInInitialize:)]) {
        CLANG_PRAGMA("clang diagnostic push")
        CLANG_PRAGMA("clang diagnostic ignored \"-Wdeprecated-declarations\"")
        [instance webProcessPlugInInitialize:[WKWebProcessPlugInController _shared]];
        CLANG_PRAGMA("clang diagnostic pop")
    }

    return true;
#else
    return false;
#endif
}

void InjectedBundle::activateMacFontAscentHack()
{
}

} // namespace WebKit
