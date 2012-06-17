/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhttpnetworkrequest_p.h"
#include "private/qnoncontiguousbytedevice_p.h"

#ifndef QT_NO_HTTP

QT_BEGIN_NAMESPACE

QHttpNetworkRequestPrivate::QHttpNetworkRequestPrivate(QHttpNetworkRequest::Operation op,
        QHttpNetworkRequest::Priority pri, const QUrl &newUrl)
    : QHttpNetworkHeaderPrivate(newUrl), operation(op), priority(pri), uploadByteDevice(0),
      autoDecompress(false), pipeliningAllowed(false), withCredentials(true)
{
}

QHttpNetworkRequestPrivate::QHttpNetworkRequestPrivate(const QHttpNetworkRequestPrivate &other)
    : QHttpNetworkHeaderPrivate(other)
{
    operation = other.operation;
    priority = other.priority;
    uploadByteDevice = other.uploadByteDevice;
    autoDecompress = other.autoDecompress;
    pipeliningAllowed = other.pipeliningAllowed;
    customVerb = other.customVerb;
    withCredentials = other.withCredentials;
    ssl = other.ssl;
}

QHttpNetworkRequestPrivate::~QHttpNetworkRequestPrivate()
{
}

bool QHttpNetworkRequestPrivate::operator==(const QHttpNetworkRequestPrivate &other) const
{
    return QHttpNetworkHeaderPrivate::operator==(other)
        && (operation == other.operation)
        && (ssl == other.ssl)
        && (uploadByteDevice == other.uploadByteDevice);
}

QByteArray QHttpNetworkRequestPrivate::methodName() const
{
    switch (operation) {
    case QHttpNetworkRequest::Get:
        return "GET";
        break;
    case QHttpNetworkRequest::Head:
        return "HEAD";
        break;
    case QHttpNetworkRequest::Post:
        return "POST";
        break;
    case QHttpNetworkRequest::Options:
        return "OPTIONS";
        break;
    case QHttpNetworkRequest::Put:
        return "PUT";
        break;
    case QHttpNetworkRequest::Delete:
        return "DELETE";
        break;
    case QHttpNetworkRequest::Trace:
        return "TRACE";
        break;
    case QHttpNetworkRequest::Connect:
        return "CONNECT";
        break;
    case QHttpNetworkRequest::Custom:
        return customVerb;
        break;
    default:
        break;
    }
    return QByteArray();
}

QByteArray QHttpNetworkRequestPrivate::uri(bool throughProxy) const
{
    QUrl::FormattingOptions format(QUrl::RemoveFragment);

    // for POST, query data is send as content
    if (operation == QHttpNetworkRequest::Post && !uploadByteDevice)
        format |= QUrl::RemoveQuery;
    // for requests through proxy, the Request-URI contains full url
    if (throughProxy)
        format |= QUrl::RemoveUserInfo;
    else
        format |= QUrl::RemoveScheme | QUrl::RemoveAuthority;
    QByteArray uri = url.toEncoded(format);
    if (uri.isEmpty() || (throughProxy && url.path().isEmpty()))
        uri += '/';
    return uri;
}

QByteArray QHttpNetworkRequestPrivate::header(const QHttpNetworkRequest &request, bool throughProxy)
{
    QList<QPair<QByteArray, QByteArray> > fields = request.header();
    QByteArray ba;
    ba.reserve(40 + fields.length()*25); // very rough lower bound estimation

    ba += request.d->methodName();
    ba += ' ';
    ba += request.d->uri(throughProxy);

    ba += " HTTP/";
    ba += QByteArray::number(request.majorVersion());
    ba += '.';
    ba += QByteArray::number(request.minorVersion());
    ba += "\r\n";

    QList<QPair<QByteArray, QByteArray> >::const_iterator it = fields.constBegin();
    QList<QPair<QByteArray, QByteArray> >::const_iterator endIt = fields.constEnd();
    for (; it != endIt; ++it) {
        ba += it->first;
        ba += ": ";
        ba += it->second;
        ba += "\r\n";
    }
    if (request.d->operation == QHttpNetworkRequest::Post) {
        // add content type, if not set in the request
        if (request.headerField("content-type").isEmpty()) {
            qWarning("content-type missing in HTTP POST, defaulting to application/octet-stream");
            ba += "Content-Type: application/octet-stream\r\n";
        }
        if (!request.d->uploadByteDevice && request.d->url.hasQuery()) {
            QByteArray query = request.d->url.encodedQuery();
            ba += "Content-Length: ";
            ba += QByteArray::number(query.size());
            ba += "\r\n\r\n";
            ba += query;
        } else {
            ba += "\r\n";
        }
    } else {
        ba += "\r\n";
    }
     return ba;
}


// QHttpNetworkRequest

QHttpNetworkRequest::QHttpNetworkRequest(const QUrl &url, Operation operation, Priority priority)
    : d(new QHttpNetworkRequestPrivate(operation, priority, url))
{
}

QHttpNetworkRequest::QHttpNetworkRequest(const QHttpNetworkRequest &other)
    : QHttpNetworkHeader(other), d(other.d)
{
}

QHttpNetworkRequest::~QHttpNetworkRequest()
{
}

QUrl QHttpNetworkRequest::url() const
{
    return d->url;
}
void QHttpNetworkRequest::setUrl(const QUrl &url)
{
    d->url = url;
}

bool QHttpNetworkRequest::isSsl() const
{
    return d->ssl;
}
void QHttpNetworkRequest::setSsl(bool s)
{
    d->ssl = s;
}

qint64 QHttpNetworkRequest::contentLength() const
{
    return d->contentLength();
}

void QHttpNetworkRequest::setContentLength(qint64 length)
{
    d->setContentLength(length);
}

QList<QPair<QByteArray, QByteArray> > QHttpNetworkRequest::header() const
{
    return d->fields;
}

QByteArray QHttpNetworkRequest::headerField(const QByteArray &name, const QByteArray &defaultValue) const
{
    return d->headerField(name, defaultValue);
}

void QHttpNetworkRequest::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    d->setHeaderField(name, data);
}

QHttpNetworkRequest &QHttpNetworkRequest::operator=(const QHttpNetworkRequest &other)
{
    d = other.d;
    return *this;
}

bool QHttpNetworkRequest::operator==(const QHttpNetworkRequest &other) const
{
    return d->operator==(*other.d);
}

QHttpNetworkRequest::Operation QHttpNetworkRequest::operation() const
{
    return d->operation;
}

void QHttpNetworkRequest::setOperation(Operation operation)
{
    d->operation = operation;
}

QByteArray QHttpNetworkRequest::customVerb() const
{
    return d->customVerb;
}

void QHttpNetworkRequest::setCustomVerb(const QByteArray &customVerb)
{
    d->customVerb = customVerb;
}

QHttpNetworkRequest::Priority QHttpNetworkRequest::priority() const
{
    return d->priority;
}

void QHttpNetworkRequest::setPriority(Priority priority)
{
    d->priority = priority;
}

bool QHttpNetworkRequest::isPipeliningAllowed() const
{
    return d->pipeliningAllowed;
}

void QHttpNetworkRequest::setPipeliningAllowed(bool b)
{
    d->pipeliningAllowed = b;
}

bool QHttpNetworkRequest::withCredentials() const
{
    return d->withCredentials;
}

void QHttpNetworkRequest::setWithCredentials(bool b)
{
    d->withCredentials = b;
}

void QHttpNetworkRequest::setUploadByteDevice(QNonContiguousByteDevice *bd)
{
    d->uploadByteDevice = bd;
}

QNonContiguousByteDevice* QHttpNetworkRequest::uploadByteDevice() const
{
    return d->uploadByteDevice;
}

int QHttpNetworkRequest::majorVersion() const
{
    return 1;
}

int QHttpNetworkRequest::minorVersion() const
{
    return 1;
}


QT_END_NAMESPACE

#endif

