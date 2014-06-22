/*
 * Copyright (C) 2007 Staikos Computing Services Inc. <info@staikos.net>
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 INdT - Instituto Nokia de Tecnologia
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
#include "PlatformStrategiesQt.h"

#include "Chrome.h"
#include "ChromeClientQt.h"
#include "QWebPageAdapter.h"
#include "qwebhistoryinterface.h"
#include "qwebpluginfactory.h"

#include <IntSize.h>
#include <NotImplemented.h>
#include <Page.h>
#include <PageGroup.h>
#include <PlatformCookieJar.h>
#include <PluginDatabase.h>
#include <QCoreApplication>
#include <QLocale>
#include <wtf/MathExtras.h>

using namespace WebCore;

void PlatformStrategiesQt::initialize()
{
    DEFINE_STATIC_LOCAL(PlatformStrategiesQt, platformStrategies, ());
    Q_UNUSED(platformStrategies);
}

PlatformStrategiesQt::PlatformStrategiesQt()
{
    setPlatformStrategies(this);
}


CookiesStrategy* PlatformStrategiesQt::createCookiesStrategy()
{
    return this;
}

DatabaseStrategy* PlatformStrategiesQt::createDatabaseStrategy()
{
    return this;
}

LoaderStrategy* PlatformStrategiesQt::createLoaderStrategy()
{
    return this;
}

PasteboardStrategy* PlatformStrategiesQt::createPasteboardStrategy()
{
    return 0;
}

PluginStrategy* PlatformStrategiesQt::createPluginStrategy()
{
    return this;
}

SharedWorkerStrategy* PlatformStrategiesQt::createSharedWorkerStrategy()
{
    return this;
}

StorageStrategy* PlatformStrategiesQt::createStorageStrategy()
{
    return this;
}

VisitedLinkStrategy* PlatformStrategiesQt::createVisitedLinkStrategy()
{
    return this;
}

String PlatformStrategiesQt::cookiesForDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesForDOM(session, firstParty, url);
}

void PlatformStrategiesQt::setCookiesFromDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, const String& cookieString)
{
    WebCore::setCookiesFromDOM(session, firstParty, url, cookieString);
}

bool PlatformStrategiesQt::cookiesEnabled(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesEnabled(session, firstParty, url);
}

String PlatformStrategiesQt::cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, url);
}

bool PlatformStrategiesQt::getRawCookies(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(session, firstParty, url, rawCookies);
}

void PlatformStrategiesQt::deleteCookie(const NetworkStorageSession& session, const KURL& url, const String& cookieName)
{
    WebCore::deleteCookie(session, url, cookieName);
}

void PlatformStrategiesQt::refreshPlugins()
{
    PluginDatabase::installedPlugins()->refresh();
}

void PlatformStrategiesQt::getPluginInfo(const WebCore::Page* page, Vector<WebCore::PluginInfo>& outPlugins)
{
    QWebPageAdapter* qPage = 0;
    if (!page->chrome().client()->isEmptyChromeClient())
        qPage = static_cast<ChromeClientQt*>(page->chrome().client())->m_webPage;

    QWebPluginFactory* factory;
    if (qPage && (factory = qPage->pluginFactory)) {

        QList<QWebPluginFactory::Plugin> qplugins = factory->plugins();
        for (int i = 0; i < qplugins.count(); ++i) {
            const QWebPluginFactory::Plugin& qplugin = qplugins.at(i);
            PluginInfo info;
            info.name = qplugin.name;
            info.desc = qplugin.description;

            for (int j = 0; j < qplugin.mimeTypes.count(); ++j) {
                const QWebPluginFactory::MimeType& mimeType = qplugin.mimeTypes.at(j);

                MimeClassInfo mimeInfo;
                mimeInfo.type = mimeType.name;
                mimeInfo.desc = mimeType.description;
                for (int k = 0; k < mimeType.fileExtensions.count(); ++k)
                    mimeInfo.extensions.append(mimeType.fileExtensions.at(k));

                info.mimes.append(mimeInfo);
            }
            outPlugins.append(info);
        }
    }

    PluginDatabase* db = PluginDatabase::installedPlugins();
    const Vector<PluginPackage*> &plugins = db->plugins();

    for (size_t i = 0; i < plugins.size(); ++i) {
        PluginInfo info;
        PluginPackage* package = plugins[i];

        info.name = package->name();
        info.file = package->fileName();
        info.desc = package->description();

        const MIMEToDescriptionsMap& mimeToDescriptions = package->mimeToDescriptions();
        MIMEToDescriptionsMap::const_iterator end = mimeToDescriptions.end();
        for (MIMEToDescriptionsMap::const_iterator it = mimeToDescriptions.begin(); it != end; ++it) {
            MimeClassInfo mime;

            mime.type = it->key;
            mime.desc = it->value;
            mime.extensions = package->mimeToExtensions().get(mime.type);

            info.mimes.append(mime);
        }

        outPlugins.append(info);
    }

}

// VisitedLinkStrategy

bool PlatformStrategiesQt::isLinkVisited(Page* page, LinkHash hash, const KURL& baseURL, const AtomicString& attributeURL)
{
    ASSERT(hash);

    Vector<UChar, 512> url;
    visitedURL(baseURL, attributeURL, url);

    // If the Qt4.4 interface for the history is used, we will have to fallback
    // to the old global history.
    QWebHistoryInterface* iface = QWebHistoryInterface::defaultInterface();
    if (iface)
        return iface->historyContains(QString(reinterpret_cast<QChar*>(url.data()), url.size()));

    return page->group().isLinkVisited(hash);
}

void PlatformStrategiesQt::addVisitedLink(Page* page, LinkHash hash)
{
    page->group().addVisitedLinkHash(hash);
}
