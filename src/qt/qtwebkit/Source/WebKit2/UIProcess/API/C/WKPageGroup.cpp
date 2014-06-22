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

#include "config.h"
#include "WKPageGroup.h"

#include "WKAPICast.h"
#include "WebPageGroup.h"
#include "WebPreferences.h"

using namespace WebKit;

WKTypeID WKPageGroupGetTypeID()
{
    return toAPI(WebPageGroup::APIType);
}

WKPageGroupRef WKPageGroupCreateWithIdentifier(WKStringRef identifier)
{
    RefPtr<WebPageGroup> pageGroup = WebPageGroup::create(toWTFString(identifier));
    return toAPI(pageGroup.release().leakRef());
}

WKStringRef WKPageGroupCopyIdentifier(WKPageGroupRef pageGroupRef)
{
    return toCopiedAPI(toImpl(pageGroupRef)->identifier());
}

void WKPageGroupSetPreferences(WKPageGroupRef pageGroupRef, WKPreferencesRef preferencesRef)
{
    toImpl(pageGroupRef)->setPreferences(toImpl(preferencesRef));
}

WKPreferencesRef WKPageGroupGetPreferences(WKPageGroupRef pageGroupRef)
{
    return toAPI(toImpl(pageGroupRef)->preferences());
}

void WKPageGroupAddUserStyleSheet(WKPageGroupRef pageGroupRef, WKStringRef sourceRef, WKURLRef baseURL, WKArrayRef whitelistedURLPatterns, WKArrayRef blacklistedURLPatterns, WKUserContentInjectedFrames injectedFrames)
{
    toImpl(pageGroupRef)->addUserStyleSheet(toWTFString(sourceRef), toWTFString(baseURL), toImpl(whitelistedURLPatterns), toImpl(blacklistedURLPatterns), toUserContentInjectedFrames(injectedFrames), WebCore::UserStyleUserLevel);
}

void WKPageGroupRemoveAllUserStyleSheets(WKPageGroupRef pageGroupRef)
{
    toImpl(pageGroupRef)->removeAllUserStyleSheets();
}

void WKPageGroupAddUserScript(WKPageGroupRef pageGroupRef, WKStringRef sourceRef, WKURLRef baseURL, WKArrayRef whitelistedURLPatterns, WKArrayRef blacklistedURLPatterns, WKUserContentInjectedFrames injectedFrames, WKUserScriptInjectionTime injectionTime)
{
    toImpl(pageGroupRef)->addUserScript(toWTFString(sourceRef), toWTFString(baseURL), toImpl(whitelistedURLPatterns), toImpl(blacklistedURLPatterns), toUserContentInjectedFrames(injectedFrames), toUserScriptInjectionTime(injectionTime));
}

void WKPageGroupRemoveAllUserScripts(WKPageGroupRef pageGroupRef)
{
    toImpl(pageGroupRef)->removeAllUserScripts();
}
