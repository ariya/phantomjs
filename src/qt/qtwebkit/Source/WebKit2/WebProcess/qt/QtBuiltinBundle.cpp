/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
#include "QtBuiltinBundle.h"

#include "QtBuiltinBundlePage.h"
#include "WKBundlePage.h"
#include "WKNumber.h"
#include "WKString.h"
#include "WKStringQt.h"
#include "WKType.h"
#include <wtf/PassOwnPtr.h>

namespace WebKit {

QtBuiltinBundle::~QtBuiltinBundle()
{
    // For OwnPtr's sake.
}

QtBuiltinBundle& QtBuiltinBundle::shared()
{
    static QtBuiltinBundle& shared = *new QtBuiltinBundle;
    return shared;
}

void QtBuiltinBundle::initialize(WKBundleRef bundle)
{
    m_bundle = bundle;

    WKBundleClient client;
    memset(&client, 0, sizeof(WKBundleClient));
    client.version = kWKBundleClientCurrentVersion;
    client.clientInfo = this;
    client.didCreatePage = QtBuiltinBundle::didCreatePage;
    client.willDestroyPage = QtBuiltinBundle::willDestroyPage;
    client.didReceiveMessageToPage = QtBuiltinBundle::didReceiveMessageToPage;
    WKBundleSetClient(m_bundle, &client);
}

void QtBuiltinBundle::didCreatePage(WKBundleRef, WKBundlePageRef page, const void* clientInfo)
{
    static_cast<QtBuiltinBundle*>(const_cast<void*>(clientInfo))->didCreatePage(page);
}

void QtBuiltinBundle::willDestroyPage(WKBundleRef, WKBundlePageRef page, const void* clientInfo)
{
    static_cast<QtBuiltinBundle*>(const_cast<void*>(clientInfo))->willDestroyPage(page);
}

void QtBuiltinBundle::didReceiveMessageToPage(WKBundleRef, WKBundlePageRef page, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
{
    static_cast<QtBuiltinBundle*>(const_cast<void*>(clientInfo))->didReceiveMessageToPage(page, messageName, messageBody);
}

void QtBuiltinBundle::didCreatePage(WKBundlePageRef page)
{
    m_pages.add(page, adoptPtr(new QtBuiltinBundlePage(this, page)));
}

void QtBuiltinBundle::willDestroyPage(WKBundlePageRef page)
{
    m_pages.remove(page);
}

void QtBuiltinBundle::didReceiveMessageToPage(WKBundlePageRef page, WKStringRef messageName, WKTypeRef messageBody)
{
    if (WKStringIsEqualToUTF8CString(messageName, "MessageToNavigatorQtObject"))
        handleMessageToNavigatorQtObject(page, messageBody);
    else if (WKStringIsEqualToUTF8CString(messageName, "SetNavigatorQtObjectEnabled"))
        handleSetNavigatorQtObjectEnabled(page, messageBody);
}

void QtBuiltinBundle::handleMessageToNavigatorQtObject(WKBundlePageRef page, WKTypeRef messageBody)
{
    ASSERT(messageBody);
    ASSERT(WKGetTypeID(messageBody) == WKStringGetTypeID());
    WKStringRef contents = static_cast<WKStringRef>(messageBody);

    QtBuiltinBundlePage* bundlePage = m_pages.get(page);
    if (!bundlePage)
        return;
    bundlePage->didReceiveMessageToNavigatorQtObject(contents);
}

void QtBuiltinBundle::handleSetNavigatorQtObjectEnabled(WKBundlePageRef page, WKTypeRef messageBody)
{
    ASSERT(messageBody);
    ASSERT(WKGetTypeID(messageBody) == WKBooleanGetTypeID());
    WKBooleanRef enabled = static_cast<WKBooleanRef>(messageBody);

    QtBuiltinBundlePage* bundlePage = m_pages.get(page);
    if (!bundlePage)
        return;
    bundlePage->setNavigatorQtObjectEnabled(enabled);
}

} // namespace WebKit
