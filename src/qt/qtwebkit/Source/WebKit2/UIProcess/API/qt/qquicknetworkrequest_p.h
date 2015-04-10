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

#ifndef qquicknetworkrequest_p_h
#define qquicknetworkrequest_p_h

#include "QtNetworkRequestData.h"
#include "wtf/RefPtr.h"
#include "qwebkitglobal.h"
#include <QObject>
#include <QtQml/qqmllist.h>
#include <QtQuick/qquickitem.h>

class QWEBKIT_EXPORT QQuickNetworkRequest : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url)

public:
    QQuickNetworkRequest(QObject* parent);

    void setNetworkRequestData(WTF::PassRefPtr<WebKit::QtRefCountedNetworkRequestData> data);

    QUrl url() const;

private:
    WTF::RefPtr<WebKit::QtRefCountedNetworkRequestData> m_networkRequestData;
};

QML_DECLARE_TYPE(QQuickNetworkRequest)

#endif // qquicknetworkrequest_p_h

