/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"
#include "NavigatorBase.h"

#include "NetworkStateNotifier.h"
#include "PlatformString.h"
#if OS(LINUX)
#include "sys/utsname.h"
#include <wtf/StdLibExtras.h>
#endif

#ifndef WEBCORE_NAVIGATOR_PLATFORM
#if OS(MAC_OS_X) && (CPU(PPC) || CPU(PPC64))
#define WEBCORE_NAVIGATOR_PLATFORM "MacPPC"
#elif OS(MAC_OS_X) && (CPU(X86) || CPU(X86_64))
#define WEBCORE_NAVIGATOR_PLATFORM "MacIntel"
#elif OS(WINDOWS)
#define WEBCORE_NAVIGATOR_PLATFORM "Win32"
#elif OS(SYMBIAN)
#define WEBCORE_NAVIGATOR_PLATFORM "Symbian"
#else
#define WEBCORE_NAVIGATOR_PLATFORM ""
#endif
#endif // ifndef WEBCORE_NAVIGATOR_PLATFORM

#ifndef WEBCORE_NAVIGATOR_PRODUCT
#define WEBCORE_NAVIGATOR_PRODUCT "Gecko"
#endif // ifndef WEBCORE_NAVIGATOR_PRODUCT

#ifndef WEBCORE_NAVIGATOR_PRODUCT_SUB
#define WEBCORE_NAVIGATOR_PRODUCT_SUB "20030107"
#endif // ifndef WEBCORE_NAVIGATOR_PRODUCT_SUB

#ifndef WEBCORE_NAVIGATOR_VENDOR
#define WEBCORE_NAVIGATOR_VENDOR "Apple Computer, Inc."
#endif // ifndef WEBCORE_NAVIGATOR_VENDOR

#ifndef WEBCORE_NAVIGATOR_VENDOR_SUB
#define WEBCORE_NAVIGATOR_VENDOR_SUB ""
#endif // ifndef WEBCORE_NAVIGATOR_VENDOR_SUB


namespace WebCore {

NavigatorBase::~NavigatorBase()
{
}

String NavigatorBase::appName() const
{
    return "Netscape";
}

String NavigatorBase::appVersion() const
{
    // Version is everything in the user agent string past the "Mozilla/" prefix.
    const String& agent = userAgent();
    return agent.substring(agent.find('/') + 1);
}

String NavigatorBase::platform() const
{
#if OS(LINUX)
    if (String("") != WEBCORE_NAVIGATOR_PLATFORM)
        return WEBCORE_NAVIGATOR_PLATFORM;
    struct utsname osname;
    DEFINE_STATIC_LOCAL(String, platformName, (uname(&osname) >= 0 ? String(osname.sysname) + String(" ") + String(osname.machine) : ""));
    return platformName;
#else
    return WEBCORE_NAVIGATOR_PLATFORM;
#endif
}

String NavigatorBase::appCodeName() const
{
    return "Mozilla";
}

String NavigatorBase::product() const
{
    return WEBCORE_NAVIGATOR_PRODUCT;
}

String NavigatorBase::productSub() const
{
    return WEBCORE_NAVIGATOR_PRODUCT_SUB;
}

String NavigatorBase::vendor() const
{
    return WEBCORE_NAVIGATOR_VENDOR;
}

String NavigatorBase::vendorSub() const
{
    return WEBCORE_NAVIGATOR_VENDOR_SUB;
}

bool NavigatorBase::onLine() const
{
    return networkStateNotifier().onLine();
}

} // namespace WebCore
