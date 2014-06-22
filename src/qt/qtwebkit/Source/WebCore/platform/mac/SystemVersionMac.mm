/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SystemVersionMac.h"

namespace WebCore {

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080

static NSString *createSystemMarketingVersion()
{
    // Can't use -[NSProcessInfo operatingSystemVersionString] because it has too much stuff we don't want.
    NSString *systemLibraryPath = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSSystemDomainMask, YES) objectAtIndex:0];
    NSString *systemVersionPlistPath = [systemLibraryPath stringByAppendingPathComponent:@"CoreServices/SystemVersion.plist"];
    NSDictionary *systemVersionInfo = [NSDictionary dictionaryWithContentsOfFile:systemVersionPlistPath];
    return [[systemVersionInfo objectForKey:@"ProductVersion"] copy];
}

#else

static inline int callGestalt(OSType selector)
{
    SInt32 value = 0;
    Gestalt(selector, &value);
    return value;
}

static NSString *createSystemMarketingVersion()
{
    // Can't use -[NSProcessInfo operatingSystemVersionString] because it has too much stuff we don't want.
    int major = callGestalt(gestaltSystemVersionMajor);
    ASSERT(major);

    int minor = callGestalt(gestaltSystemVersionMinor);
    int bugFix = callGestalt(gestaltSystemVersionBugFix);
    if (bugFix)
        return [[NSString alloc] initWithFormat:@"%d.%d.%d", major, minor, bugFix];
    if (minor)
        return [[NSString alloc] initWithFormat:@"%d.%d", major, minor];
    return [[NSString alloc] initWithFormat:@"%d", major];
}

#endif // __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080


NSString *systemMarketingVersion()
{
    static NSString *version = createSystemMarketingVersion();
    return version;
}

}
