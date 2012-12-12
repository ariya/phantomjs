/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHTTPMULTIPART_H
#define QHTTPMULTIPART_H

#include <QtCore/QSharedDataPointer>
#include <QtCore/QByteArray>
#include <QtNetwork/QNetworkRequest>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

class QHttpPartPrivate;
class QHttpMultiPart;

class Q_NETWORK_EXPORT QHttpPart
{
public:
    QHttpPart();
    QHttpPart(const QHttpPart &other);
    ~QHttpPart();
    QHttpPart &operator=(const QHttpPart &other);
    bool operator==(const QHttpPart &other) const;
    inline bool operator!=(const QHttpPart &other) const
    { return !operator==(other); }

    void setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value);
    void setRawHeader(const QByteArray &headerName, const QByteArray &headerValue);

    void setBody(const QByteArray &body);
    void setBodyDevice(QIODevice *device);

private:
    QSharedDataPointer<QHttpPartPrivate> d;

    friend class QHttpMultiPartIODevice;
};

class QHttpMultiPartPrivate;

class Q_NETWORK_EXPORT QHttpMultiPart : public QObject
{
    Q_OBJECT

public:

    enum ContentType {
        MixedType,
        RelatedType,
        FormDataType,
        AlternativeType
    };

    QHttpMultiPart(QObject *parent = 0);
    QHttpMultiPart(ContentType contentType, QObject *parent = 0);
    ~QHttpMultiPart();

    void append(const QHttpPart &httpPart);

    void setContentType(ContentType contentType);

    QByteArray boundary() const;
    void setBoundary(const QByteArray &boundary);

private:
    Q_DECLARE_PRIVATE(QHttpMultiPart)
    Q_DISABLE_COPY(QHttpMultiPart)

    friend class QNetworkAccessManager;
    friend class QNetworkAccessManagerPrivate;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QHTTPMULTIPART_H
