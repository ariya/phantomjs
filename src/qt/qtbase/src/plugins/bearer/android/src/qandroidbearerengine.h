/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QANDROIDBEARERENGINE_H
#define QANDROIDBEARERENGINE_H

#include "../../qbearerengine_impl.h"

#include <QAbstractEventDispatcher>
#include <QAbstractNativeEventFilter>
#include <QtCore/private/qjni_p.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QNetworkConfigurationPrivate;
class QNetworkSessionPrivate;
class AndroidConnectivityManager;

class QAndroidBearerEngine : public QBearerEngineImpl
{
    Q_OBJECT

public:
    explicit QAndroidBearerEngine(QObject *parent = 0);
    ~QAndroidBearerEngine() Q_DECL_OVERRIDE;

    QString getInterfaceFromId(const QString &id) Q_DECL_OVERRIDE;
    bool hasIdentifier(const QString &id) Q_DECL_OVERRIDE;
    void connectToId(const QString &id) Q_DECL_OVERRIDE;
    void disconnectFromId(const QString &id) Q_DECL_OVERRIDE;
    QNetworkSession::State sessionStateForId(const QString &id) Q_DECL_OVERRIDE;
    QNetworkConfigurationManager::Capabilities capabilities() const Q_DECL_OVERRIDE;
    QNetworkSessionPrivate *createSessionBackend() Q_DECL_OVERRIDE;
    QNetworkConfigurationPrivatePointer defaultConfiguration() Q_DECL_OVERRIDE;
    bool requiresPolling() const Q_DECL_OVERRIDE;
    quint64 bytesWritten(const QString &id) Q_DECL_OVERRIDE;
    quint64 bytesReceived(const QString &id) Q_DECL_OVERRIDE;
    quint64 startTime(const QString &id) Q_DECL_OVERRIDE;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

private Q_SLOTS:
    void updateConfigurations();

private:
    QJNIObjectPrivate m_networkReceiver;
    AndroidConnectivityManager *m_connectivityManager;
    QMap<QString, QString> m_configurationInterface;
};


QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif // QANDROIDBEARERENGINE_H
