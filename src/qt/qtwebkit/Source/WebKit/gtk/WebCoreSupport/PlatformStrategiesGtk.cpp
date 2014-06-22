/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "PlatformStrategiesGtk.h"

#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformCookieJar.h"
#include "PluginDatabase.h"
#include "PluginPackage.h"

using namespace WebCore;

void PlatformStrategiesGtk::initialize()
{
    DEFINE_STATIC_LOCAL(PlatformStrategiesGtk, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
}

PlatformStrategiesGtk::PlatformStrategiesGtk()
{
}

CookiesStrategy* PlatformStrategiesGtk::createCookiesStrategy()
{
    return this;
}

DatabaseStrategy* PlatformStrategiesGtk::createDatabaseStrategy()
{
    return this;
}

LoaderStrategy* PlatformStrategiesGtk::createLoaderStrategy()
{
    return this;
}

PasteboardStrategy* PlatformStrategiesGtk::createPasteboardStrategy()
{
    // This is currently used only by Mac code.
    notImplemented();
    return 0;
}

PluginStrategy* PlatformStrategiesGtk::createPluginStrategy()
{
    return this;
}

SharedWorkerStrategy* PlatformStrategiesGtk::createSharedWorkerStrategy()
{
    return this;
}

StorageStrategy* PlatformStrategiesGtk::createStorageStrategy()
{
    return this;
}

VisitedLinkStrategy* PlatformStrategiesGtk::createVisitedLinkStrategy()
{
    return this;
}

// CookiesStrategy
String PlatformStrategiesGtk::cookiesForDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesForDOM(session, firstParty, url);
}

void PlatformStrategiesGtk::setCookiesFromDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, const String& cookieString)
{
    WebCore::setCookiesFromDOM(session, firstParty, url, cookieString);
}

bool PlatformStrategiesGtk::cookiesEnabled(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesEnabled(session, firstParty, url);
}

String PlatformStrategiesGtk::cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, url);
}

bool PlatformStrategiesGtk::getRawCookies(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(session, firstParty, url, rawCookies);
}

void PlatformStrategiesGtk::deleteCookie(const NetworkStorageSession& session, const KURL& url, const String& cookieName)
{
    WebCore::deleteCookie(session, url, cookieName);
}

// PluginStrategy
void PlatformStrategiesGtk::refreshPlugins()
{
    PluginDatabase::installedPlugins()->refresh();
}

void PlatformStrategiesGtk::getPluginInfo(const Page* page, Vector<PluginInfo>& outPlugins)
{
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
}

// VisitedLinkStrategy
bool PlatformStrategiesGtk::isLinkVisited(Page* page, LinkHash hash, const KURL&, const AtomicString&)
{
    return page->group().isLinkVisited(hash);
}

void PlatformStrategiesGtk::addVisitedLink(Page* page, LinkHash hash)
{
    page->group().addVisitedLinkHash(hash);
}
