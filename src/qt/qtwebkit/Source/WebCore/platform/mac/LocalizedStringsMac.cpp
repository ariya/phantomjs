/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "LocalizedStrings.h"

#include <CoreFoundation/CFBundle.h>
#include <wtf/Assertions.h>
#include <wtf/MainThread.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

String localizedString(const char* key)
{
    static CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.WebCore"));

#if !PLATFORM(IOS)
    // Can be called on a dispatch queue when initializing strings on iOS.
    // See LoadWebLocalizedStrings and <rdar://problem/7902473>.
    ASSERT(isMainThread());
#endif

    RetainPtr<CFStringRef> keyString = adoptCF(CFStringCreateWithCStringNoCopy(0, key, kCFStringEncodingUTF8, kCFAllocatorNull));
    CFStringRef notFound = CFSTR("localized string not found");
    RetainPtr<CFStringRef> result;
    if (bundle) {
        result = adoptCF(CFBundleCopyLocalizedString(bundle, keyString.get(), notFound, 0));
        ASSERT_WITH_MESSAGE(result.get() != notFound, "could not find localizable string %s in bundle", key);
    } else
        result = notFound;

    return String(result.get());
}

} // namespace WebCore
