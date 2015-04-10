/*
 * Copyright (C) 2003, 2005, 2006, 2010, 2011 Apple Inc. All rights reserved.
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
#import "Language.h"

#import "BlockExceptions.h"
#import "WebCoreSystemInterface.h"
#import <wtf/Assertions.h>
#import <wtf/MainThread.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/WTFString.h>

using namespace WebCore;

static BOOL useCachedPreferredLanguages;

@interface WebLanguageChangeObserver : NSObject {
}
@end

@implementation WebLanguageChangeObserver

+ (void)_webkit_languagePreferencesDidChange
{
    ASSERT(isMainThread());

    useCachedPreferredLanguages = NO;

    languageDidChange();
}

@end

namespace WebCore {

static String httpStyleLanguageCode(NSString *languageCode)
{
    ASSERT(isMainThread());

    // Look up the language code using CFBundle.
    RetainPtr<CFStringRef> preferredLanguageCode = adoptCF(wkCopyCFLocalizationPreferredName((CFStringRef)languageCode));

    if (preferredLanguageCode)
        languageCode = (NSString *)preferredLanguageCode.get();

    // Make the string lowercase.
    NSString *lowercaseLanguageCode = [languageCode lowercaseString];

    // Turn a '_' into a '-' if it appears after a 2-letter language code.
    if ([lowercaseLanguageCode length] >= 3 && [lowercaseLanguageCode characterAtIndex:2] == '_') {
        RetainPtr<NSMutableString> mutableLanguageCode = adoptNS([lowercaseLanguageCode mutableCopy]);
        [mutableLanguageCode.get() replaceCharactersInRange:NSMakeRange(2, 1) withString:@"-"];
        return mutableLanguageCode.get();
    }

    return lowercaseLanguageCode;
}

Vector<String> platformUserPreferredLanguages()
{
    DEFINE_STATIC_LOCAL(Vector<String>, userPreferredLanguages, ());

    ASSERT(isMainThread());

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    if (!useCachedPreferredLanguages) {
        useCachedPreferredLanguages = YES;
        userPreferredLanguages.clear();

        RetainPtr<CFArrayRef> languages = adoptCF(CFLocaleCopyPreferredLanguages());
        CFIndex languageCount = CFArrayGetCount(languages.get());
        if (!languageCount)
            userPreferredLanguages.append("en");
        else {
            for (CFIndex i = 0; i < languageCount; i++)
                userPreferredLanguages.append(httpStyleLanguageCode((NSString *)CFArrayGetValueAtIndex(languages.get(), i)));
        }
    }

#if !PLATFORM(IOS)
    static bool languageChangeObserverAdded;
    if (!languageChangeObserverAdded) {
        [[NSDistributedNotificationCenter defaultCenter] addObserver:[WebLanguageChangeObserver self]
                                                            selector:@selector(_webkit_languagePreferencesDidChange)
                                                                name:@"AppleLanguagePreferencesChangedNotification"
                                                              object:nil];
        languageChangeObserverAdded = true;
    }
#endif // !PLATFORM(IOS)
    
    return userPreferredLanguages;

    END_BLOCK_OBJC_EXCEPTIONS;

    return Vector<String>();
}

}
