/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETWORKREPLY_P_H
#define QNETWORKREPLY_P_H

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

#include "qnetworkrequest.h"
#include "qnetworkrequest_p.h"
#include "qnetworkreply.h"
#include "QtCore/qpointer.h"
#include <QtCore/QElapsedTimer>
#include "private/qiodevice_p.h"

QT_BEGIN_NAMESPACE

class QNetworkReplyPrivate: public QIODevicePrivate, public QNetworkHeadersPrivate
{
public:
    enum State {
        Idle,               // The reply is idle.
        Buffering,          // The reply is buffering outgoing data.
        Working,            // The reply is uploading/downloading data.
        Finished,           // The reply has finished.
        Aborted,            // The reply has been aborted.
        WaitingForSession,  // The reply is waiting for the session to open before connecting.
        Reconnecting        // The reply will reconnect to once roaming has completed.
    };

    QNetworkReplyPrivate();
    QNetworkRequest request;
    QUrl url;
    QPointer<QNetworkAccessManager> manager;
    qint64 readBufferMaxSize;
    QElapsedTimer downloadProgressSignalChoke;
    QElapsedTimer uploadProgressSignalChoke;
    const static int progressSignalInterval;
    QNetworkAccessManager::Operation operation;
    QNetworkReply::NetworkError errorCode;
    bool isFinished;

    static inline void setManager(QNetworkReply *reply, QNetworkAccessManager *manager)
    { reply->d_func()->manager = manager; }

    Q_DECLARE_PUBLIC(QNetworkReply)
};

QT_END_NAMESPACE

#endif
