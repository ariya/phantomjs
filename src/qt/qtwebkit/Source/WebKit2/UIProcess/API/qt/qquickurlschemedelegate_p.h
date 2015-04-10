/*
 * Copyright (C) 2011 Zeno Albisser <zeno@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef qquickurlschemedelegate_p_h
#define qquickurlschemedelegate_p_h

#include "qwebkitglobal.h"
#include <QObject>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

class QQuickNetworkRequest;
class QQuickNetworkReply;

class QWEBKIT_EXPORT QQuickUrlSchemeDelegate : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString scheme READ scheme WRITE setScheme NOTIFY schemeChanged)
    Q_PROPERTY(QQuickNetworkRequest* request READ request)
    Q_PROPERTY(QQuickNetworkReply* reply READ reply)

public:
    QQuickUrlSchemeDelegate(QObject* parent = 0);
    QString scheme() const;
    void setScheme(const QString& scheme);
    QQuickNetworkRequest* request() const;
    QQuickNetworkReply* reply() const;

Q_SIGNALS:
    void schemeChanged();
    void receivedRequest();

private:
    QString m_scheme;
    QQuickNetworkRequest* m_request;
    QQuickNetworkReply* m_reply;
};

QML_DECLARE_TYPE(QQuickUrlSchemeDelegate)

class QQuickQrcSchemeDelegate : public QQuickUrlSchemeDelegate {
    Q_OBJECT
public:
    QQuickQrcSchemeDelegate(const QUrl& url);
    void readResourceAndSend();

private:
    QString m_fileName;
};

#endif // qquickurlschemedelegate_p_h


