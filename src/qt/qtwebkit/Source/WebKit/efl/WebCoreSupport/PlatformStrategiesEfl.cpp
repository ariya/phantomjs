/*
    Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
    Copyright (C) 2008 Trolltech ASA
    Copyright (C) 2008 Collabora Ltd. All rights reserved.
    Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2011 Samsung Electronics
    Copyright (C) 2012 Intel Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "PlatformStrategiesEfl.h"

#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformCookieJar.h"
#include "PluginDatabase.h"
#include "PluginPackage.h"

using namespace WebCore;

void PlatformStrategiesEfl::initialize()
{
    DEFINE_STATIC_LOCAL(PlatformStrategiesEfl, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
}

PlatformStrategiesEfl::PlatformStrategiesEfl()
{
}

CookiesStrategy* PlatformStrategiesEfl::createCookiesStrategy()
{
    return this;
}

DatabaseStrategy* PlatformStrategiesEfl::createDatabaseStrategy()
{
    return this;
}

LoaderStrategy* PlatformStrategiesEfl::createLoaderStrategy()
{
    return this;
}

PasteboardStrategy* PlatformStrategiesEfl::createPasteboardStrategy()
{
    notImplemented();
    return 0;
}

PluginStrategy* PlatformStrategiesEfl::createPluginStrategy()
{
    return this;
}

SharedWorkerStrategy* PlatformStrategiesEfl::createSharedWorkerStrategy()
{
    return this;
}

VisitedLinkStrategy* PlatformStrategiesEfl::createVisitedLinkStrategy()
{
    return this;
}

StorageStrategy* PlatformStrategiesEfl::createStorageStrategy()
{
    return this;
}

// CookiesStrategy
String PlatformStrategiesEfl::cookiesForDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesForDOM(session, firstParty, url);
}

void PlatformStrategiesEfl::setCookiesFromDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, const String& cookieString)
{
    WebCore::setCookiesFromDOM(session, firstParty, url, cookieString);
}

bool PlatformStrategiesEfl::cookiesEnabled(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesEnabled(session, firstParty, url);
}

String PlatformStrategiesEfl::cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, url);
}

bool PlatformStrategiesEfl::getRawCookies(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(session, firstParty, url, rawCookies);
}

void PlatformStrategiesEfl::deleteCookie(const NetworkStorageSession& session, const KURL& url, const String& cookieName)
{
    WebCore::deleteCookie(session, url, cookieName);
}

void PlatformStrategiesEfl::refreshPlugins()
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    PluginDatabase::installedPlugins()->refresh();
#endif
}

void PlatformStrategiesEfl::getPluginInfo(const Page*, Vector<PluginInfo>& outPlugins)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    PluginDatabase::installedPlugins()->refresh();
    const Vector<PluginPackage*>& plugins = PluginDatabase::installedPlugins()->plugins();
    outPlugins.resize(plugins.size());

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
bool PlatformStrategiesEfl::isLinkVisited(Page* page, LinkHash hash, const KURL&, const AtomicString&)
{
    return page->group().isLinkVisited(hash);
}

void PlatformStrategiesEfl::addVisitedLink(Page* page, LinkHash hash)
{
    page->group().addVisitedLinkHash(hash);
}
