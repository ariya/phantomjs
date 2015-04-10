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
#import "WKBrowsingContextGroup.h"
#import "WKBrowsingContextGroupPrivate.h"

#import "WKArray.h"
#import "WKPageGroup.h"
#import "WKPreferences.h"
#import "WKRetainPtr.h"
#import "WKStringCF.h"
#import "WKURL.h"
#import "WKURLCF.h"
#import <wtf/Vector.h>

@interface WKBrowsingContextGroupData : NSObject {
@public
    WKRetainPtr<WKPageGroupRef> _pageGroupRef;
}
@end

@implementation WKBrowsingContextGroupData
@end

@implementation WKBrowsingContextGroup

- (id)initWithIdentifier:(NSString *)identifier
{
    self = [super init];
    if (!self)
        return nil;

    _data = [[WKBrowsingContextGroupData alloc] init];
    _data->_pageGroupRef = adoptWK(WKPageGroupCreateWithIdentifier(adoptWK(WKStringCreateWithCFString((CFStringRef)identifier)).get()));

    // Give the WKBrowsingContextGroup a identifier-less preferences, so that they
    // don't get automatically written to the disk. The automatic writing has proven
    // confusing to users of the API.
    WKRetainPtr<WKPreferencesRef> preferences = adoptWK(WKPreferencesCreate());
    WKPageGroupSetPreferences(_data->_pageGroupRef.get(), preferences.get());

    return self;
}

- (void)dealloc
{
    [_data release];
    [super dealloc];
}

- (BOOL)allowsJavaScript
{
    return WKPreferencesGetJavaScriptEnabled(WKPageGroupGetPreferences(self._pageGroupRef));
}

- (void)setAllowsJavaScript:(BOOL)allowsJavaScript
{
    WKPreferencesSetJavaScriptEnabled(WKPageGroupGetPreferences(self._pageGroupRef), allowsJavaScript);
}

- (BOOL)allowsJavaScriptMarkup
{
    return WKPreferencesGetJavaScriptMarkupEnabled(WKPageGroupGetPreferences(self._pageGroupRef));
}

- (void)setAllowsJavaScriptMarkup:(BOOL)allowsJavaScriptMarkup
{
    WKPreferencesSetJavaScriptMarkupEnabled(WKPageGroupGetPreferences(self._pageGroupRef), allowsJavaScriptMarkup);
}

- (BOOL)allowsPlugIns
{
    return WKPreferencesGetPluginsEnabled(WKPageGroupGetPreferences(self._pageGroupRef));
}

- (void)setAllowsPlugIns:(BOOL)allowsPlugIns
{
    WKPreferencesSetPluginsEnabled(WKPageGroupGetPreferences(self._pageGroupRef), allowsPlugIns);
}

static WKArrayRef createWKArray(NSArray *array)
{
    NSUInteger count = [array count];
    if (count == 0)
        return WKArrayRef();

    Vector<WKTypeRef> stringVector;
    stringVector.reserveInitialCapacity(count);
    for (NSUInteger i = 0; i < count; ++i) {
        id entry = [array objectAtIndex:i];
        if ([entry isKindOfClass:[NSString class]])
            stringVector.uncheckedAppend(WKStringCreateWithCFString((CFStringRef)entry));
            
    }

    return WKArrayCreateAdoptingValues(stringVector.data(), stringVector.size());
}

-(void)addUserStyleSheet:(NSString *)source baseURL:(NSURL *)baseURL whitelistedURLPatterns:(NSArray *)whitelist blacklistedURLPatterns:(NSArray *)blacklist mainFrameOnly:(BOOL)mainFrameOnly
{
    if (!source)
        CRASH();

    WKRetainPtr<WKStringRef> wkSource = adoptWK(WKStringCreateWithCFString((CFStringRef)source));
    WKRetainPtr<WKURLRef> wkBaseURL = adoptWK(WKURLCreateWithCFURL((CFURLRef)baseURL));
    WKRetainPtr<WKArrayRef> wkWhitelist = adoptWK(createWKArray(whitelist));
    WKRetainPtr<WKArrayRef> wkBlacklist = adoptWK(createWKArray(blacklist));
    WKUserContentInjectedFrames injectedFrames = mainFrameOnly ? kWKInjectInTopFrameOnly : kWKInjectInAllFrames;

    WKPageGroupAddUserStyleSheet(self._pageGroupRef, wkSource.get(), wkBaseURL.get(), wkWhitelist.get(), wkBlacklist.get(), injectedFrames);
}

- (void)removeAllUserStyleSheets
{
    WKPageGroupRemoveAllUserStyleSheets(self._pageGroupRef);
}

- (void)addUserScript:(NSString *)source baseURL:(NSURL *)baseURL whitelistedURLPatterns:(NSArray *)whitelist blacklistedURLPatterns:(NSArray *)blacklist injectionTime:(WKUserScriptInjectionTime)injectionTime mainFrameOnly:(BOOL)mainFrameOnly
{
    if (!source)
        CRASH();

    WKRetainPtr<WKStringRef> wkSource = adoptWK(WKStringCreateWithCFString((CFStringRef)source));
    WKRetainPtr<WKURLRef> wkBaseURL = adoptWK(WKURLCreateWithCFURL((CFURLRef)baseURL));
    WKRetainPtr<WKArrayRef> wkWhitelist = adoptWK(createWKArray(whitelist));
    WKRetainPtr<WKArrayRef> wkBlacklist = adoptWK(createWKArray(blacklist));
    WKUserContentInjectedFrames injectedFrames = mainFrameOnly ? kWKInjectInTopFrameOnly : kWKInjectInAllFrames;

    WKPageGroupAddUserScript(self._pageGroupRef, wkSource.get(), wkBaseURL.get(), wkWhitelist.get(), wkBlacklist.get(), injectedFrames, injectionTime);
}

- (void)removeAllUserScripts
{
    WKPageGroupRemoveAllUserScripts(self._pageGroupRef);
}

@end

@implementation WKBrowsingContextGroup (Private)

- (WKPageGroupRef)_pageGroupRef
{
    return _data->_pageGroupRef.get();
}

@end
