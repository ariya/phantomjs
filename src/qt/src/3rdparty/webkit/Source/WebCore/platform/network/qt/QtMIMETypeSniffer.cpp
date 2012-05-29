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
#include "QtMIMETypeSniffer.h"

#include "MIMESniffing.h"
#include <QCoreApplication>
#include <QNetworkReply>

QtMIMETypeSniffer::QtMIMETypeSniffer(QNetworkReply* reply, const QString& advertisedMimeType, bool isSupportedImageType)
    : QObject(0)
    , m_reply(reply)
    , m_mimeType(advertisedMimeType)
    , m_sniffer(advertisedMimeType.toLatin1().constData(), isSupportedImageType)
    , m_isFinished(false)
{
    m_isFinished = !m_sniffer.isValid() || sniff();
    if (m_isFinished)
        return;

    connect(m_reply, SIGNAL(readyRead()), this, SLOT(trySniffing()));
    connect(m_reply, SIGNAL(finished()), this, SLOT(trySniffing()));
}

bool QtMIMETypeSniffer::sniff()
{
    // See QNetworkReplyWrapper::setFinished().
    const bool isReplyFinished = m_reply->property("_q_isFinished").toBool();

    if (!isReplyFinished && m_reply->bytesAvailable() < m_sniffer.dataSize())
        return false;

    QByteArray data = m_reply->peek(m_sniffer.dataSize());
    const char* sniffedMimeType = m_sniffer.sniff(data.constData(), data.size());
    if (sniffedMimeType)
        m_mimeType = QString::fromLatin1(sniffedMimeType);
    return true;
}

void QtMIMETypeSniffer::trySniffing()
{
    if (!sniff())
        return;

    m_reply->disconnect(this);
    QCoreApplication::removePostedEvents(this, QEvent::MetaCall);
    m_isFinished = true;
    emit finished();
}
