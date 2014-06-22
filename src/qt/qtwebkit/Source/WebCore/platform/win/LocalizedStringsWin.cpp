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

#include "config.h"
#include "LocalizedStrings.h"

#include "WebCoreInstanceHandle.h"
#include <windows.h>
#include <wtf/Assertions.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

#if USE(CF)
#include <CoreFoundation/CFBundle.h>
#include <wtf/RetainPtr.h>
#endif

namespace WebCore {

#if USE(CF)

static CFBundleRef createWebKitBundle()
{
    if (CFBundleRef existingBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.WebKit"))) {
        CFRetain(existingBundle);
        return existingBundle;
    }

    wchar_t dllPathBuffer[MAX_PATH];
    DWORD length = ::GetModuleFileNameW(instanceHandle(), dllPathBuffer, WTF_ARRAY_LENGTH(dllPathBuffer));
    ASSERT(length);
    ASSERT(length < WTF_ARRAY_LENGTH(dllPathBuffer));

    RetainPtr<CFStringRef> dllPath = adoptCF(CFStringCreateWithCharactersNoCopy(0, reinterpret_cast<const UniChar*>(dllPathBuffer), length, kCFAllocatorNull));
    RetainPtr<CFURLRef> dllURL = adoptCF(CFURLCreateWithFileSystemPath(0, dllPath.get(), kCFURLWindowsPathStyle, false));
    RetainPtr<CFURLRef> dllDirectoryURL = adoptCF(CFURLCreateCopyDeletingLastPathComponent(0, dllURL.get()));
    RetainPtr<CFURLRef> resourcesDirectoryURL = adoptCF(CFURLCreateCopyAppendingPathComponent(0, dllDirectoryURL.get(), CFSTR("WebKit.resources"), true));

    return CFBundleCreate(0, resourcesDirectoryURL.get());
}

static CFBundleRef webKitBundle()
{
    static CFBundleRef bundle = createWebKitBundle();
    ASSERT(bundle);
    return bundle;
}

#endif // USE(CF)

String localizedString(const char* key)
{
    ASSERT(isMainThread());

#if USE(CF)
    static CFStringRef notFound = CFSTR("localized string not found");

    RetainPtr<CFStringRef> keyString = adoptCF(CFStringCreateWithCStringNoCopy(NULL, key, kCFStringEncodingUTF8, kCFAllocatorNull));
    RetainPtr<CFStringRef> result = adoptCF(CFCopyLocalizedStringWithDefaultValue(keyString.get(), 0, webKitBundle(), notFound, 0));
    ASSERT_WITH_MESSAGE(result.get() != notFound, "could not find localizable string %s in bundle", key);

    return result.get();
#else
    // FIXME: Implement localizedString() for !USE(CF).
    return String::fromUTF8(key, strlen(key));
#endif
}

} // namespace WebCore
