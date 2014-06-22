/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QNETWORKACCESSFTPBACKEND_P_H
#define QNETWORKACCESSFTPBACKEND_P_H

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

#include "qnetworkaccessbackend_p.h"
#include "qnetworkaccesscache_p.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "private/qftp_p.h"

#include "QtCore/qpointer.h"

#ifndef QT_NO_FTP

QT_BEGIN_NAMESPACE

class QNetworkAccessFtpIODevice;
class QNetworkAccessCachedFtpConnection;

class QNetworkAccessFtpBackend: public QNetworkAccessBackend
{
    Q_OBJECT
public:
    enum State {
        Idle,
        //Connecting,
        LoggingIn,
        CheckingFeatures,
        Statting,
        Transferring,
        Disconnecting
    };

    QNetworkAccessFtpBackend();
    virtual ~QNetworkAccessFtpBackend();

    virtual void open();
    virtual void closeDownstreamChannel();

    virtual void downstreamReadyWrite();

    void disconnectFromFtp();

public slots:
    void ftpConnectionReady(QNetworkAccessCache::CacheableObject *object);
    void ftpDone();
    void ftpReadyRead();
    void ftpRawCommandReply(int code, const QString &text);

private:
    friend class QNetworkAccessFtpIODevice;
    QPointer<QNetworkAccessCachedFtpConnection> ftp;
    QIODevice *uploadDevice;
    qint64 totalBytes;
    int helpId, sizeId, mdtmId;
    bool supportsSize, supportsMdtm;
    State state;
};

class QNetworkAccessFtpBackendFactory: public QNetworkAccessBackendFactory
{
public:
    virtual QStringList supportedSchemes() const Q_DECL_OVERRIDE;
    virtual QNetworkAccessBackend *create(QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &request) const;
};

QT_END_NAMESPACE

#endif // QT_NO_FTP

#endif
