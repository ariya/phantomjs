/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "WKPluginInformation.h"

#include "PluginInformation.h"
#include "WKSharedAPICast.h"
#include "WebString.h"

using namespace WebKit;

WKStringRef WKPluginInformationBundleIdentifierKey()
{
    static WebString* key = WebString::create(pluginInformationBundleIdentifierKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationBundleVersionKey()
{
    static WebString* key = WebString::create(pluginInformationBundleVersionKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationBundleShortVersionKey()
{
    static WebString* key = WebString::create(pluginInformationBundleShortVersionKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationPathKey()
{
    static WebString* key = WebString::create(pluginInformationPathKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationDisplayNameKey()
{
    static WebString* key = WebString::create(pluginInformationDisplayNameKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationDefaultLoadPolicyKey()
{
    static WebString* key = WebString::create(pluginInformationDefaultLoadPolicyKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationUpdatePastLastBlockedVersionIsKnownAvailableKey()
{
    static WebString* key = WebString::create(pluginInformationUpdatePastLastBlockedVersionIsKnownAvailableKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationHasSandboxProfileKey()
{
    static WebString* key = WebString::create(pluginInformationHasSandboxProfileKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationFrameURLKey()
{
    static WebString* key = WebString::create(pluginInformationFrameURLKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationMIMETypeKey()
{
    static WebString* key = WebString::create(pluginInformationMIMETypeKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationPageURLKey()
{
    static WebString* key = WebString::create(pluginInformationPageURLKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationPluginspageAttributeURLKey()
{
    static WebString* key = WebString::create(pluginInformationPluginspageAttributeURLKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPluginInformationPluginURLKey()
{
    static WebString* key = WebString::create(pluginInformationPluginURLKey()).leakRef();
    return toAPI(key);
}

WKStringRef WKPlugInInformationReplacementObscuredKey()
{
    static WebString* key = WebString::create(plugInInformationReplacementObscuredKey()).leakRef();
    return toAPI(key);
}
