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

#ifndef QNETWORKSESSION_H
#define QNETWORKSESSION_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtNetwork/qnetworkinterface.h>
#include <QtCore/qvariant.h>
#include <QtNetwork/qnetworkconfiguration.h>

#ifndef QT_NO_BEARERMANAGEMENT

#if defined(Q_OS_WIN) && defined(interface)
#undef interface
#endif

QT_BEGIN_HEADER

#ifndef QT_MOBILITY_BEARER
#include <QtCore/qshareddata.h>
QT_BEGIN_NAMESPACE
QT_MODULE(Network)
#define QNetworkSessionExport Q_NETWORK_EXPORT
#else
#include "qmobilityglobal.h"
QTM_BEGIN_NAMESPACE
#define QNetworkSessionExport Q_BEARER_EXPORT
#endif

class QNetworkSessionPrivate;
class QNetworkSessionExport QNetworkSession : public QObject
{
    Q_OBJECT

public:
    enum State {
        Invalid = 0,
        NotAvailable,
        Connecting,
        Connected,
        Closing,
        Disconnected,
        Roaming
    };

    enum SessionError {
        UnknownSessionError = 0,
        SessionAbortedError,
        RoamingError,
        OperationNotSupportedError,
        InvalidConfigurationError
    };

    explicit QNetworkSession(const QNetworkConfiguration &connConfig, QObject *parent = 0);
    virtual ~QNetworkSession();

    bool isOpen() const;
    QNetworkConfiguration configuration() const;
#ifndef QT_NO_NETWORKINTERFACE
    QNetworkInterface interface() const;
#endif

    State state() const;
    SessionError error() const;
    QString errorString() const;
    QVariant sessionProperty(const QString &key) const;
    void setSessionProperty(const QString &key, const QVariant &value);

    quint64 bytesWritten() const;
    quint64 bytesReceived() const;
    quint64 activeTime() const;
    
    bool waitForOpened(int msecs = 30000);

public Q_SLOTS:
    void open();
    void close();
    void stop();

    //roaming related slots
    void migrate();
    void ignore();
    void accept();
    void reject();

Q_SIGNALS:
    void stateChanged(QNetworkSession::State);
    void opened();
    void closed();
    void error(QNetworkSession::SessionError);
    void preferredConfigurationChanged(const QNetworkConfiguration &config, bool isSeamless);
    void newConfigurationActivated();

protected:
    virtual void connectNotify(const char *signal);
    virtual void disconnectNotify(const char *signal);

private:
    friend class QNetworkSessionPrivate;
    QNetworkSessionPrivate *d;
};

#ifndef QT_MOBILITY_BEARER
QT_END_NAMESPACE
Q_DECLARE_METATYPE(QNetworkSession::State)
Q_DECLARE_METATYPE(QNetworkSession::SessionError)
#else
QTM_END_NAMESPACE
#endif

QT_END_HEADER

#endif // QT_NO_BEARERMANAGEMENT

#endif // QNETWORKSESSION_H
