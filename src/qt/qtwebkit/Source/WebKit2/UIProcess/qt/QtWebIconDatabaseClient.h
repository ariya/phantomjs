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


#ifndef QtWebIconDatabaseClient_h
#define QtWebIconDatabaseClient_h

#include "qwebkitglobal.h"
#include <QtCore/QObject>
#include <WKIconDatabase.h>

QT_BEGIN_NAMESPACE
class QImage;
class QUrl;
QT_END_NAMESPACE

namespace WebKit {

class QtWebIconDatabaseClient : public QObject {
    Q_OBJECT

public:
    QtWebIconDatabaseClient(WKContextRef);
    ~QtWebIconDatabaseClient();

    QImage iconImageForPageURL(const QString&);

    void retainIconForPageURL(const QString&);
    void releaseIconForPageURL(const QString&);

    static unsigned updateID();

public:
    Q_SIGNAL void iconChangedForPageURL(const QString& pageURL);

private:
    static void didChangeIconForPageURL(WKIconDatabaseRef, WKURLRef pageURL, const void* clientInfo);
    WKIconDatabaseRef m_iconDatabase;
};

} // namespace WebKit

#endif
