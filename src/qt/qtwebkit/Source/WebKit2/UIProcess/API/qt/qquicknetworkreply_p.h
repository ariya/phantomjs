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

#ifndef qquicknetworkreply_p_h
#define qquicknetworkreply_p_h

#include "QtNetworkReplyData.h"
#include "QtNetworkRequestData.h"
#include "qquickwebview_p.h"
#include "SharedMemory.h"
#include "qwebkitglobal.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QtQml/qqmllist.h>
#include <QtQuick/qquickitem.h>

class QWEBKIT_EXPORT QQuickNetworkReply : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString contentType READ contentType WRITE setContentType)
    Q_PROPERTY(QVariant data READ data WRITE setData)
    Q_ENUMS(QNetworkAccessManager::Operation)

public:
    QQuickNetworkReply(QObject* parent);
    QString contentType() const;
    void setContentType(const QString&);

    QVariant data() const;
    void setData(const QVariant& data);

    void setWebViewExperimental(QQuickWebViewExperimental*);
    WebKit::QtRefCountedNetworkRequestData* networkRequestData() const;
    void setNetworkRequestData(WTF::PassRefPtr<WebKit::QtRefCountedNetworkRequestData> data);
    WebKit::QtRefCountedNetworkReplyData* networkReplyData() const;

public Q_SLOTS:
    void send();

private:
    WTF::RefPtr<WebKit::QtRefCountedNetworkRequestData> m_networkRequestData;
    WTF::RefPtr<WebKit::QtRefCountedNetworkReplyData> m_networkReplyData;
    QVariant m_data;
    QPointer<QQuickWebViewExperimental> m_webViewExperimental;
};

QML_DECLARE_TYPE(QQuickNetworkReply)

#endif // qquicknetworkreply_p_h

