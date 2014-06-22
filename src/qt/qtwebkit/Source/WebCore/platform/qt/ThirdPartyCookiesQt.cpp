/*
 * Copyright (C) 2011 Robert Hogan <robert@roberthogan.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ThirdPartyCookiesQt.h"

#include "Cookie.h"
#include "CookieJar.h"
#include "Document.h"
#include "NetworkingContext.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>

namespace WebCore {

inline void removeTopLevelDomain(QString* domain, const QString& topLevelDomain)
{
    domain->remove(domain->length() - topLevelDomain.length(), topLevelDomain.length());
    domain->prepend(QLatin1Char('.'));
}

static bool urlsShareSameDomain(const QUrl& url, const QUrl& firstPartyUrl)
{
    QString firstPartyTLD = firstPartyUrl.topLevelDomain();
    QString requestTLD = url.topLevelDomain();

    if (firstPartyTLD != requestTLD)
        return false;

    QString firstPartyDomain = QString(firstPartyUrl.host().toLower());
    QString requestDomain = QString(url.host().toLower());

    removeTopLevelDomain(&firstPartyDomain, firstPartyTLD);
    removeTopLevelDomain(&requestDomain, requestTLD);

    if (firstPartyDomain.section(QLatin1Char('.'), -1) == requestDomain.section(QLatin1Char('.'), -1))
        return true;

    return false;
}

bool thirdPartyCookiePolicyPermits(NetworkingContext* context, const QUrl& url, const QUrl& firstPartyUrl)
{
    if (!context)
        return true;

    if (!context->networkAccessManager())
        return true;

    QNetworkCookieJar* jar = context->networkAccessManager()->cookieJar();
    if (!jar)
        return true;

    if (firstPartyUrl.isEmpty())
        return true;

    if (urlsShareSameDomain(url, firstPartyUrl))
        return true;

    return context->thirdPartyCookiePolicyPermission(url);
}

}
// vim: ts=4 sw=4 et
