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

#include "config.h"
#include "qquicknetworkrequest_p.h"

#include "QtNetworkRequestData.h"
#include "qquickwebview_p.h"

using namespace WebKit;

QQuickNetworkRequest::QQuickNetworkRequest(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(parent);
}

void QQuickNetworkRequest::setNetworkRequestData(WTF::PassRefPtr<WebKit::QtRefCountedNetworkRequestData> data)
{
    m_networkRequestData = data;
}

QUrl QQuickNetworkRequest::url() const
{
    if (m_networkRequestData)
       return QUrl(m_networkRequestData->data().m_urlString);
    return QUrl();
}

#include "moc_qquicknetworkrequest_p.cpp"

