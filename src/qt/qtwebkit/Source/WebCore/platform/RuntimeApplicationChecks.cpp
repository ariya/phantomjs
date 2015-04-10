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
#include "RuntimeApplicationChecks.h"

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#include <wtf/RetainPtr.h>
#endif

#include <wtf/text/WTFString.h>

namespace WebCore {
    
static bool mainBundleIsEqualTo(const String& bundleIdentifierString)
{
#if USE(CF)
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (!mainBundle)
        return false;

    CFStringRef bundleIdentifier = CFBundleGetIdentifier(mainBundle);
    if (!bundleIdentifier)
        return false;

    return CFStringCompare(bundleIdentifier, bundleIdentifierString.createCFString().get(), 0) == kCFCompareEqualTo;
#else
    UNUSED_PARAM(bundleIdentifierString);
    return false;
#endif
}

bool applicationIsSafari()
{
    // FIXME: For the WebProcess case, ensure that this is Safari's WebProcess.
    static bool isSafari = mainBundleIsEqualTo("com.apple.Safari") || mainBundleIsEqualTo("com.apple.WebProcess");
    return isSafari;
}

bool applicationIsAppleMail()
{
    static bool isAppleMail = mainBundleIsEqualTo("com.apple.mail");
    return isAppleMail;
}

bool applicationIsMicrosoftMessenger()
{
    static bool isMicrosoftMessenger = mainBundleIsEqualTo("com.microsoft.Messenger");
    return isMicrosoftMessenger;
}

bool applicationIsAdobeInstaller()
{
    static bool isAdobeInstaller = mainBundleIsEqualTo("com.adobe.Installers.Setup");
    return isAdobeInstaller;
}

bool applicationIsAOLInstantMessenger()
{
    static bool isAOLInstantMessenger = mainBundleIsEqualTo("com.aol.aim.desktop");
    return isAOLInstantMessenger;
}

bool applicationIsMicrosoftMyDay()
{
    static bool isMicrosoftMyDay = mainBundleIsEqualTo("com.microsoft.myday");
    return isMicrosoftMyDay;
}

bool applicationIsMicrosoftOutlook()
{
    static bool isMicrosoftOutlook = mainBundleIsEqualTo("com.microsoft.Outlook");
    return isMicrosoftOutlook;
}

bool applicationIsAperture()
{
    static bool isAperture = mainBundleIsEqualTo("com.apple.Aperture");
    return isAperture;
}

bool applicationIsVersions()
{
    static bool isVersions = mainBundleIsEqualTo("com.blackpixel.versions");
    return isVersions;
}

bool applicationIsHRBlock()
{
    static bool isHRBlock = mainBundleIsEqualTo("com.hrblock.tax.2010");
    return isHRBlock;
}

bool applicationIsSolidStateNetworksDownloader()
{
    static bool isSolidStateNetworksDownloader = mainBundleIsEqualTo("com.solidstatenetworks.awkhost");
    return isSolidStateNetworksDownloader;
}

} // namespace WebCore
