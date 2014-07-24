/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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
#include "qwebiconimageprovider_p.h"

#include "QtWebContext.h"
#include "QtWebIconDatabaseClient.h"
#include <QtCore/QUrl>
#include <QtGui/QImage>

using namespace WebKit;

QWebIconImageProvider::QWebIconImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QWebIconImageProvider::~QWebIconImageProvider()
{
}

QUrl QWebIconImageProvider::iconURLForPageURLInContext(const QString &pageURL, QtWebContext* context)
{
    QtWebIconDatabaseClient* iconDatabase = context->iconDatabase();

    // Verify that the image data is actually available before reporting back
    // a url, since clients assume that the url can be used directly.
    if (iconDatabase->iconImageForPageURL(pageURL).isNull())
        return QUrl();

    QUrl url;
    url.setScheme(QStringLiteral("image"));
    url.setHost(QWebIconImageProvider::identifier());
    // Make sure that QML doesn't show a cached previous version of the icon after it changed.
    url.setPath(QStringLiteral("/%1").arg(QtWebIconDatabaseClient::updateID()));

    // FIXME: Use QUrl::DecodedMode when landed in Qt
    url.setFragment(QString::fromLatin1(pageURL.toUtf8().toBase64()));

    // FIXME: We can't know when the icon url is no longer in use,
    // so we never release these icons. At some point we might want
    // to introduce expiry of icons to elevate this issue.
    iconDatabase->retainIconForPageURL(pageURL);

    return url;
}

QImage QWebIconImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    QString pageURL = QString::fromUtf8(QByteArray::fromBase64(id.midRef(id.indexOf('#') + 1).toLatin1()));

    QtWebIconDatabaseClient* iconDatabase = QtWebContext::defaultContext()->iconDatabase();
    Q_ASSERT(iconDatabase);

    QImage icon = iconDatabase->iconImageForPageURL(pageURL);
    Q_ASSERT(!icon.isNull());

    if (size)
        *size = icon.size();

    if (requestedSize.isValid())
        return icon.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return icon;
}
