/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

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
#ifndef _WEBDATABASE_H_
#define _WEBDATABASE_H_

#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>

#include "qwebkitglobal.h"

namespace WebCore {
    class DatabaseDetails;
}

class QWebDatabasePrivate;
class QWebSecurityOrigin;

class QWEBKIT_EXPORT QWebDatabase {
public:
    QWebDatabase(const QWebDatabase& other);
    QWebDatabase &operator=(const QWebDatabase& other);
    ~QWebDatabase();

    QString name() const;
    QString displayName() const;
    qint64 expectedSize() const;
    qint64 size() const;
    QString fileName() const;
    QWebSecurityOrigin origin() const;

    static void removeDatabase(const QWebDatabase&);
    static void removeAllDatabases();

private:
    QWebDatabase(QWebDatabasePrivate* priv);
    friend class QWebSecurityOrigin;

private:
    QExplicitlySharedDataPointer<QWebDatabasePrivate> d;
};

#endif

