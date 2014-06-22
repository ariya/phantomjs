/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "PlatformStrategiesBlackBerry.h"

#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformCookieJar.h"
#include "PluginDatabase.h"
#include "PluginPackage.h"

using namespace WebCore;

void PlatformStrategiesBlackBerry::initialize()
{
    DEFINE_STATIC_LOCAL(PlatformStrategiesBlackBerry, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
}

PlatformStrategiesBlackBerry::PlatformStrategiesBlackBerry()
{
}

CookiesStrategy* PlatformStrategiesBlackBerry::createCookiesStrategy()
{
    return this;
}

DatabaseStrategy* PlatformStrategiesBlackBerry::createDatabaseStrategy()
{
    return this;
}

LoaderStrategy* PlatformStrategiesBlackBerry::createLoaderStrategy()
{
    return this;
}

PasteboardStrategy* PlatformStrategiesBlackBerry::createPasteboardStrategy()
{
    // This is currently used only by Mac.
    notImplemented();
    return 0;
}

PluginStrategy* PlatformStrategiesBlackBerry::createPluginStrategy()
{
    return this;
}

SharedWorkerStrategy* PlatformStrategiesBlackBerry::createSharedWorkerStrategy()
{
    return this;
}

StorageStrategy* PlatformStrategiesBlackBerry::createStorageStrategy()
{
    return this;
}

VisitedLinkStrategy* PlatformStrategiesBlackBerry::createVisitedLinkStrategy()
{
    return this;
}

// CookiesStrategy
String PlatformStrategiesBlackBerry::cookiesForDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesForDOM(session, firstParty, url);
}

void PlatformStrategiesBlackBerry::setCookiesFromDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, const String& cookieString)
{
    WebCore::setCookiesFromDOM(session, firstParty, url, cookieString);
}

bool PlatformStrategiesBlackBerry::cookiesEnabled(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesEnabled(session, firstParty, url);
}

String PlatformStrategiesBlackBerry::cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, url);
}

bool PlatformStrategiesBlackBerry::getRawCookies(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(session, firstParty, url, rawCookies);
}

void PlatformStrategiesBlackBerry::deleteCookie(const NetworkStorageSession& session, const KURL& url, const String& cookieName)
{
    WebCore::deleteCookie(session, url, cookieName);
}

// PluginStrategy
void PlatformStrategiesBlackBerry::refreshPlugins()
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    PluginDatabase::installedPlugins()->refresh();
#endif
}

void PlatformStrategiesBlackBerry::getPluginInfo(const Page*, Vector<PluginInfo>& outPlugins)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    PluginDatabase* database = PluginDatabase::installedPlugins();
    const Vector<PluginPackage*> &plugins = database->plugins();

    for (size_t i = 0; i < plugins.size(); ++i) {
        PluginPackage* package = plugins[i];

        PluginInfo pluginInfo;
        pluginInfo.name = package->name();
        pluginInfo.file = package->fileName();
        pluginInfo.desc = package->description();

        const MIMEToDescriptionsMap& mimeToDescriptions = package->mimeToDescriptions();
        MIMEToDescriptionsMap::const_iterator end = mimeToDescriptions.end();
        for (MIMEToDescriptionsMap::const_iterator it = mimeToDescriptions.begin(); it != end; ++it) {
            MimeClassInfo mime;
            mime.type = it->key;
            mime.desc = it->value;
            mime.extensions = package->mimeToExtensions().get(mime.type);
            pluginInfo.mimes.append(mime);
        }

        outPlugins.append(pluginInfo);
    }
#else
    UNUSED_PARAM(outPlugins);
#endif
}

// VisitedLinkStrategy
bool PlatformStrategiesBlackBerry::isLinkVisited(Page* page, LinkHash hash, const KURL&, const AtomicString&)
{
    return page->group().isLinkVisited(hash);
}

void PlatformStrategiesBlackBerry::addVisitedLink(Page* page, LinkHash hash)
{
    page->group().addVisitedLinkHash(hash);
}
