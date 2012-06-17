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

//#define QTCPSOCKET_DEBUG

/*! 
    \class QTcpSocket

    \brief The QTcpSocket class provides a TCP socket.

    \reentrant
    \ingroup network
    \inmodule QtNetwork

    TCP (Transmission Control Protocol) is a reliable,
    stream-oriented, connection-oriented transport protocol. It is
    especially well suited for continuous transmission of data.

    QTcpSocket is a convenience subclass of QAbstractSocket that
    allows you to establish a TCP connection and transfer streams of
    data. See the QAbstractSocket documentation for details.

    \bold{Note:} TCP sockets cannot be opened in QIODevice::Unbuffered mode.

    \section1 Symbian Platform Security Requirements

    On Symbian, processes which use this class must have the
    \c NetworkServices platform security capability. If the client
    process lacks this capability, it will result in a panic.

    Platform security capabilities are added via the
    \l{qmake-variable-reference.html#target-capability}{TARGET.CAPABILITY}
    qmake variable.

    \sa QTcpServer, QUdpSocket, QFtp, QNetworkAccessManager,
    {Fortune Server Example}, {Fortune Client Example},
    {Threaded Fortune Server Example}, {Blocking Fortune Client Example},
    {Loopback Example}, {Torrent Example}
*/

#include "qlist.h"
#include "qtcpsocket_p.h"
#include "qtcpsocket.h"
#include "qhostaddress.h"

QT_BEGIN_NAMESPACE

/*!
    Creates a QTcpSocket object in state \c UnconnectedState.

    \a parent is passed on to the QObject constructor.

    \sa socketType()
*/
QTcpSocket::QTcpSocket(QObject *parent)
    : QAbstractSocket(TcpSocket, *new QTcpSocketPrivate, parent)
{
#if defined(QTCPSOCKET_DEBUG)
    qDebug("QTcpSocket::QTcpSocket()");
#endif
    d_func()->isBuffered = true;
}

/*!
    Destroys the socket, closing the connection if necessary.

    \sa close()
*/

QTcpSocket::~QTcpSocket()
{
#if defined(QTCPSOCKET_DEBUG)
    qDebug("QTcpSocket::~QTcpSocket()");
#endif
}

/*!
    \internal
*/
QTcpSocket::QTcpSocket(QTcpSocketPrivate &dd, QObject *parent)
    : QAbstractSocket(TcpSocket, dd, parent)
{
    d_func()->isBuffered = true;
}

QT_END_NAMESPACE
