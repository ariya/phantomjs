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

#ifndef QNETWORKCONFIGURATIONMANAGERPRIVATE_H
#define QNETWORKCONFIGURATIONMANAGERPRIVATE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qnetworkconfigmanager.h"
#include "qnetworkconfiguration_p.h"

#include <QtCore/qmutex.h>
#include <QtCore/qset.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QBearerEngine;
class QTimer;

class Q_NETWORK_EXPORT QNetworkConfigurationManagerPrivate : public QObject
{
    Q_OBJECT

public:
    QNetworkConfigurationManagerPrivate();
    virtual ~QNetworkConfigurationManagerPrivate();

    QNetworkConfiguration defaultConfiguration() const;
    QList<QNetworkConfiguration> allConfigurations(QNetworkConfiguration::StateFlags filter) const;
    QNetworkConfiguration configurationFromIdentifier(const QString &identifier) const;

    bool isOnline() const;

    QNetworkConfigurationManager::Capabilities capabilities() const;

    void performAsyncConfigurationUpdate();

    QList<QBearerEngine *> engines() const;

    void enablePolling();
    void disablePolling();

    void initialize();
    void cleanup();
public Q_SLOTS:
    void updateConfigurations();

    static void addPostRoutine();

Q_SIGNALS:
    void configurationAdded(const QNetworkConfiguration &config);
    void configurationRemoved(const QNetworkConfiguration &config);
    void configurationChanged(const QNetworkConfiguration &config);
    void configurationUpdateComplete();
    void onlineStateChanged(bool isOnline);

private Q_SLOTS:
    void configurationAdded(QNetworkConfigurationPrivatePointer ptr);
    void configurationRemoved(QNetworkConfigurationPrivatePointer ptr);
    void configurationChanged(QNetworkConfigurationPrivatePointer ptr);

    void pollEngines();


private:
    Q_INVOKABLE void startPolling();
    QTimer *pollTimer;
    QThread *bearerThread;

private:
    mutable QMutex mutex;

    QList<QBearerEngine *> sessionEngines;

    QSet<QString> onlineConfigurations;

    QSet<QBearerEngine *> pollingEngines;
    QSet<QBearerEngine *> updatingEngines;
    int forcedPolling;
    bool updating;

    bool firstUpdate;
};

Q_NETWORK_EXPORT QNetworkConfigurationManagerPrivate *qNetworkConfigurationManagerPrivate();

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif // QNETWORKCONFIGURATIONMANAGERPRIVATE_H
