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
#include "QtWebIconDatabaseClient.h"

#include "QtWebContext.h"
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <WKContext.h>
#include <WKContextPrivate.h>
#include <WKIconDatabaseQt.h>
#include <WKRetainPtr.h>
#include <WKStringQt.h>
#include <WKURLQt.h>

namespace WebKit {

static unsigned s_updateId = 0;

static inline QtWebIconDatabaseClient* toQtWebIconDatabaseClient(const void* clientInfo)
{
    ASSERT(clientInfo);
    return reinterpret_cast<QtWebIconDatabaseClient*>(const_cast<void*>(clientInfo));
}

QtWebIconDatabaseClient::QtWebIconDatabaseClient(WKContextRef context)
{
    m_iconDatabase = WKContextGetIconDatabase(context);

    WKIconDatabaseClient iconDatabaseClient;
    memset(&iconDatabaseClient, 0, sizeof(WKIconDatabaseClient));
    iconDatabaseClient.version = kWKIconDatabaseClientCurrentVersion;
    iconDatabaseClient.clientInfo = this;
    iconDatabaseClient.didChangeIconForPageURL = didChangeIconForPageURL;
    WKIconDatabaseSetIconDatabaseClient(m_iconDatabase, &iconDatabaseClient);
    // Triggers the startup of the icon database.
    WKRetainPtr<WKStringRef> path = adoptWK(WKStringCreateWithQString(QtWebContext::preparedStoragePath(QtWebContext::IconDatabaseStorage)));
    WKContextSetIconDatabasePath(context, path.get());
}

QtWebIconDatabaseClient::~QtWebIconDatabaseClient()
{
    WKIconDatabaseClose(m_iconDatabase);
    WKIconDatabaseSetIconDatabaseClient(m_iconDatabase, 0);
}

unsigned QtWebIconDatabaseClient::updateID()
{
    return s_updateId;
}

void QtWebIconDatabaseClient::didChangeIconForPageURL(WKIconDatabaseRef, WKURLRef pageURL, const void* clientInfo)
{
    ++s_updateId;
    emit toQtWebIconDatabaseClient(clientInfo)->iconChangedForPageURL(WKURLCopyQString(pageURL));
}

QImage QtWebIconDatabaseClient::iconImageForPageURL(const QString& pageURL)
{
    return WKIconDatabaseTryGetQImageForURL(m_iconDatabase, adoptWK(WKURLCreateWithQString(pageURL)).get());
}

void QtWebIconDatabaseClient::retainIconForPageURL(const QString& pageURL)
{
    WKIconDatabaseRetainIconForURL(m_iconDatabase, adoptWK(WKURLCreateWithQString(pageURL)).get());
}

void QtWebIconDatabaseClient::releaseIconForPageURL(const QString& pageURL)
{
    WKIconDatabaseReleaseIconForURL(m_iconDatabase, adoptWK(WKURLCreateWithQString(pageURL)).get());
}

} // namespace WebKit

#include "moc_QtWebIconDatabaseClient.cpp"

