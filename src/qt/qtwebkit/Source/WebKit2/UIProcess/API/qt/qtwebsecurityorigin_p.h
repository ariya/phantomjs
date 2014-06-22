/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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
#ifndef qtwebsecurityorigin_p_h
#define qtwebsecurityorigin_p_h

#include "qwebkitglobal.h"

#include <QtCore/QDataStream>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/qshareddata.h>

class QWEBKIT_EXPORT QtWebSecurityOrigin : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString scheme READ scheme CONSTANT)
    Q_PROPERTY(QString host READ host CONSTANT)
    Q_PROPERTY(int port READ port CONSTANT)
    Q_PROPERTY(QString path READ path CONSTANT)

public:
    QtWebSecurityOrigin(QObject* parent = 0);
    virtual ~QtWebSecurityOrigin();

    QString scheme() const;
    QString host() const;
    int port() const;
    QString path() const;

    // Used to set security information in a permission request event (e.g.
    // geolocation permission)
    void setScheme(const QString& scheme) { m_url.setScheme(scheme); }
    void setHost(const QString& host) { m_url.setHost(host); }
    void setPath(const QString& path) { m_url.setPath(path); }
    void setPort(int port) { m_url.setPort(port); }

private:
    QUrl m_url;
};

#endif // qtwebsecurityorigin_p_h
