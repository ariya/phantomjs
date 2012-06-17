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

#include "qlocalserver.h"
#include "qlocalserver_p.h"
#include "qlocalsocket.h"
#include "qlocalsocket_p.h"

#include <qhostaddress.h>
#include <qsettings.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

void QLocalServerPrivate::init()
{
    Q_Q(QLocalServer);
    q->connect(&tcpServer, SIGNAL(newConnection()), SLOT(_q_onNewConnection()));
}

bool QLocalServerPrivate::listen(const QString &requestedServerName)
{
    if (!tcpServer.listen(QHostAddress::LocalHost))
        return false;

    const QLatin1String prefix("QLocalServer/");
    if (requestedServerName.startsWith(prefix))
        fullServerName = requestedServerName;
    else
        fullServerName = prefix + requestedServerName;

    QSettings settings(QLatin1String("Trolltech"), QLatin1String("Qt"));
    if (settings.contains(fullServerName)) {
        qWarning("QLocalServer::listen: server name is already in use.");
        tcpServer.close();
        return false;
    }

    settings.setValue(fullServerName, tcpServer.serverPort());
    return true;
}

void QLocalServerPrivate::closeServer()
{
    QSettings settings(QLatin1String("Trolltech"), QLatin1String("Qt"));
    if (fullServerName == QLatin1String("QLocalServer"))
        settings.setValue(fullServerName, QVariant());
    else
        settings.remove(fullServerName);
    tcpServer.close();
}

void QLocalServerPrivate::waitForNewConnection(int msec, bool *timedOut)
{
    if (pendingConnections.isEmpty())
        tcpServer.waitForNewConnection(msec, timedOut);
    else if (timedOut)
        *timedOut = false;
}

void QLocalServerPrivate::_q_onNewConnection()
{
    Q_Q(QLocalServer);
    QTcpSocket* tcpSocket = tcpServer.nextPendingConnection();
    if (!tcpSocket) {
        qWarning("QLocalServer: no pending connection");
        return;
    }

    tcpSocket->setParent(q);
    const quintptr socketDescriptor = tcpSocket->socketDescriptor();
    q->incomingConnection(socketDescriptor);
}

bool QLocalServerPrivate::removeServer(const QString &name)
{
    const QLatin1String prefix("QLocalServer/");
    QString serverName;
    if (name.startsWith(prefix))
        serverName = name;
    else
        serverName = prefix + name;

    QSettings settings(QLatin1String("Trolltech"), QLatin1String("Qt"));
    if (settings.contains(serverName))
        settings.remove(serverName);

    return true;
}

QT_END_NAMESPACE
