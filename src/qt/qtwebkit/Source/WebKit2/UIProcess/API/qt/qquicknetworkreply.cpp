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
#include "qquicknetworkreply_p.h"

#include "QtNetworkReplyData.h"
#include "QtNetworkRequestData.h"
#include "qquickwebview_p.h"
#include <QDateTime>

using namespace WebKit;

QQuickNetworkReply::QQuickNetworkReply(QObject* parent)
    : QObject(parent)
    , m_networkReplyData(adoptRef(new WebKit::QtRefCountedNetworkReplyData))
{
    Q_ASSERT(parent);
}

QString QQuickNetworkReply::contentType() const
{
    return m_networkReplyData->data().m_contentType;
}

void QQuickNetworkReply::setContentType(const QString& contentType)
{
    m_networkReplyData->data().m_contentType = contentType;
}

QVariant QQuickNetworkReply::data() const
{
    return m_data;
}

void QQuickNetworkReply::setData(const QVariant& data)
{
    m_data = data;
}

void QQuickNetworkReply::send()
{
    if (m_data.isNull())
        return;

    uint64_t smLength = 0;
    const void* ptrData = 0;
    QString stringData;
    QByteArray byteArrayData;
    if (m_data.type() == QVariant::String) {
        stringData = m_data.toString();
        ptrData = reinterpret_cast<const void*>(stringData.constData());
        smLength = sizeof(QChar) * stringData.length();
        setContentType(QLatin1String("text/html; charset=utf-16"));
    } else {
        if (!m_data.canConvert<QByteArray>())
            return;
        byteArrayData = m_data.toByteArray();
        ptrData = byteArrayData.data();
        smLength = byteArrayData.size();
    }

    if (contentType().isEmpty()) {
        qWarning("QQuickNetworkReply::send - Cannot send raw data without a content type being specified!");
        return;
    }

    WTF::RefPtr<WebKit::SharedMemory> sharedMemory = SharedMemory::create(smLength);
    if (!sharedMemory)
        return;
    // The size of the allocated shared memory can be bigger than requested.
    // Usually the size will be rounded up to the next multiple of a page size.
    memcpy(sharedMemory->data(), ptrData, smLength);

    if (sharedMemory->createHandle(m_networkReplyData->data().m_dataHandle, SharedMemory::ReadOnly)) {
        m_networkReplyData->data().m_contentLength = smLength;
        if (m_webViewExperimental)
            m_webViewExperimental.data()->sendApplicationSchemeReply(this);
    }

    // After sending the reply data, we have to reinitialize the m_networkReplyData,
    // to make sure we have a fresh SharesMemory::Handle.
    m_networkReplyData = adoptRef(new WebKit::QtRefCountedNetworkReplyData);
}

void QQuickNetworkReply::setWebViewExperimental(QQuickWebViewExperimental* webViewExperimental)
{
    m_webViewExperimental = webViewExperimental;
}

WebKit::QtRefCountedNetworkRequestData* QQuickNetworkReply::networkRequestData() const
{
    return m_networkRequestData.get();
}

void QQuickNetworkReply::setNetworkRequestData(WTF::PassRefPtr<WebKit::QtRefCountedNetworkRequestData> data)
{
    m_networkRequestData = data;
    m_networkReplyData->data().m_replyUuid = m_networkRequestData->data().m_replyUuid;
}

WebKit::QtRefCountedNetworkReplyData* QQuickNetworkReply::networkReplyData() const
{
    return m_networkReplyData.get();
}

#include "moc_qquicknetworkreply_p.cpp"
