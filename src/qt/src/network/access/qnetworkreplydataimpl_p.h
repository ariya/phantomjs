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

#ifndef QNETWORKREPLYDATAIMPL_H
#define QNETWORKREPLYDATAIMPL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkaccessmanager.h"
#include <QBuffer>

QT_BEGIN_NAMESPACE


class QNetworkReplyDataImplPrivate;
class QNetworkReplyDataImpl: public QNetworkReply
{
    Q_OBJECT
public:
    QNetworkReplyDataImpl(QObject *parent, const QNetworkRequest &req, const QNetworkAccessManager::Operation op);
    ~QNetworkReplyDataImpl();
    virtual void abort();

    // reimplemented from QNetworkReply
    virtual void close();
    virtual qint64 bytesAvailable() const;
    virtual bool isSequential () const;
    qint64 size() const;

    virtual qint64 readData(char *data, qint64 maxlen);

    Q_DECLARE_PRIVATE(QNetworkReplyDataImpl)
};

class QNetworkReplyDataImplPrivate: public QNetworkReplyPrivate
{
public:
    QNetworkReplyDataImplPrivate();
    ~QNetworkReplyDataImplPrivate();

    QPair<QString, QByteArray> decodeDataUrlResult;
    QBuffer decodedData;

    Q_DECLARE_PUBLIC(QNetworkReplyDataImpl)
};

QT_END_NAMESPACE

#endif // QNETWORKREPLYDATAIMPL_H
