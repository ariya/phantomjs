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
#include "qquickurlschemedelegate_p.h"

#include "qquicknetworkreply_p.h"
#include "qquicknetworkrequest_p.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QUrl>

QQuickUrlSchemeDelegate::QQuickUrlSchemeDelegate(QObject* parent)
    : QObject(parent)
    , m_request(new QQuickNetworkRequest(this))
    , m_reply(new QQuickNetworkReply(this))
{ }

QString QQuickUrlSchemeDelegate::scheme() const
{
    return m_scheme;
}

void QQuickUrlSchemeDelegate::setScheme(const QString& scheme)
{
    m_scheme = scheme;
    emit schemeChanged();
}

QQuickNetworkRequest* QQuickUrlSchemeDelegate::request() const
{
    return m_request;
}

QQuickNetworkReply* QQuickUrlSchemeDelegate::reply() const
{
    return m_reply;
}

QQuickQrcSchemeDelegate::QQuickQrcSchemeDelegate(const QUrl& url)
    : QQuickUrlSchemeDelegate()
    , m_fileName(QLatin1Char(':') + url.path())
{
}

void QQuickQrcSchemeDelegate::readResourceAndSend()
{
    QFile file(m_fileName);
    QFileInfo fileInfo(file);
    if (fileInfo.isDir() || !file.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
        return;

    QByteArray fileData(file.readAll());
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFileNameAndData(m_fileName, fileData);
    file.close();

    reply()->setData(fileData);
    reply()->setContentType(mimeType.name());
    reply()->send();
}

#include "moc_qquickurlschemedelegate_p.cpp"
