/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 * Copyright (C) 2012, Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "NavigatorContentUtils.h"

#if ENABLE(NAVIGATOR_CONTENT_UTILS)

#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "Navigator.h"
#include "Page.h"
#include <wtf/HashSet.h>

namespace WebCore {

static HashSet<String>* protocolWhitelist;

static void initProtocolHandlerWhitelist()
{
    protocolWhitelist = new HashSet<String>;
#if !PLATFORM(BLACKBERRY)
    static const char* protocols[] = {
        "irc",
        "geo",
        "mailto",
        "magnet",
        "mms",
        "news",
        "nntp",
        "sip",
        "sms",
        "smsto",
        "ssh",
        "tel",
        "urn",
        "webcal",
        "xmpp"
    };
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(protocols); ++i)
        protocolWhitelist->add(protocols[i]);
#endif
}

static bool verifyCustomHandlerURL(const String& baseURL, const String& url, ExceptionCode& ec)
{
    // The specification requires that it is a SYNTAX_ERR if the "%s" token is
    // not present.
    static const char token[] = "%s";
    int index = url.find(token);
    if (-1 == index) {
        ec = SYNTAX_ERR;
        return false;
    }

    // It is also a SYNTAX_ERR if the custom handler URL, as created by removing
    // the "%s" token and prepending the base url, does not resolve.
    String newURL = url;
    newURL.remove(index, WTF_ARRAY_LENGTH(token) - 1);

    KURL base(ParsedURLString, baseURL);
    KURL kurl(base, newURL);

    if (kurl.isEmpty() || !kurl.isValid()) {
        ec = SYNTAX_ERR;
        return false;
    }

    return true;
}

static bool isProtocolWhitelisted(const String& scheme)
{
    if (!protocolWhitelist)
        initProtocolHandlerWhitelist();
    return protocolWhitelist->contains(scheme);
}

static bool verifyProtocolHandlerScheme(const String& scheme, ExceptionCode& ec)
{
    if (scheme.startsWith("web+")) {
        if (isValidProtocol(scheme))
            return true;
        ec = SECURITY_ERR;
        return false;
    }

    if (isProtocolWhitelisted(scheme))
        return true;
    ec = SECURITY_ERR;
    return false;
}

NavigatorContentUtils* NavigatorContentUtils::from(Page* page)
{
    return static_cast<NavigatorContentUtils*>(RefCountedSupplement<Page, NavigatorContentUtils>::from(page, NavigatorContentUtils::supplementName()));
}

NavigatorContentUtils::~NavigatorContentUtils()
{
}

PassRefPtr<NavigatorContentUtils> NavigatorContentUtils::create(NavigatorContentUtilsClient* client)
{
    return adoptRef(new NavigatorContentUtils(client));
}

void NavigatorContentUtils::registerProtocolHandler(Navigator* navigator, const String& scheme, const String& url, const String& title, ExceptionCode& ec)
{
    if (!navigator->frame())
        return;

    Document* document = navigator->frame()->document();
    if (!document)
        return;

    String baseURL = document->baseURL().baseAsString();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return;

    if (!verifyProtocolHandlerScheme(scheme, ec))
        return;

    NavigatorContentUtils::from(navigator->frame()->page())->client()->registerProtocolHandler(scheme, baseURL, url, navigator->frame()->displayStringModifiedByEncoding(title));
}

#if ENABLE(CUSTOM_SCHEME_HANDLER)
static String customHandlersStateString(const NavigatorContentUtilsClient::CustomHandlersState state)
{
    DEFINE_STATIC_LOCAL(const String, newHandler, (ASCIILiteral("new")));
    DEFINE_STATIC_LOCAL(const String, registeredHandler, (ASCIILiteral("registered")));
    DEFINE_STATIC_LOCAL(const String, declinedHandler, (ASCIILiteral("declined")));

    switch (state) {
    case NavigatorContentUtilsClient::CustomHandlersNew:
        return newHandler;
    case NavigatorContentUtilsClient::CustomHandlersRegistered:
        return registeredHandler;
    case NavigatorContentUtilsClient::CustomHandlersDeclined:
        return declinedHandler;
    }

    ASSERT_NOT_REACHED();
    return String();
}

String NavigatorContentUtils::isProtocolHandlerRegistered(Navigator* navigator, const String& scheme, const String& url, ExceptionCode& ec)
{
    DEFINE_STATIC_LOCAL(const String, declined, ("declined"));

    if (!navigator->frame())
        return declined;

    Document* document = navigator->frame()->document();
    String baseURL = document->baseURL().baseAsString();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return declined;

    if (!verifyProtocolHandlerScheme(scheme, ec))
        return declined;

    return customHandlersStateString(NavigatorContentUtils::from(navigator->frame()->page())->client()->isProtocolHandlerRegistered(scheme, baseURL, url));
}

void NavigatorContentUtils::unregisterProtocolHandler(Navigator* navigator, const String& scheme, const String& url, ExceptionCode& ec)
{
    if (!navigator->frame())
        return;

    Document* document = navigator->frame()->document();
    String baseURL = document->baseURL().baseAsString();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return;

    if (!verifyProtocolHandlerScheme(scheme, ec))
        return;

    NavigatorContentUtils::from(navigator->frame()->page())->client()->unregisterProtocolHandler(scheme, baseURL, url);
}
#endif

const char* NavigatorContentUtils::supplementName()
{
    return "NavigatorContentUtils";
}

void provideNavigatorContentUtilsTo(Page* page, NavigatorContentUtilsClient* client)
{
    RefCountedSupplement<Page, NavigatorContentUtils>::provideTo(page, NavigatorContentUtils::supplementName(), NavigatorContentUtils::create(client));
}

} // namespace WebCore

#endif // ENABLE(NAVIGATOR_CONTENT_UTILS)

