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

#ifndef QBEARERENGINE_P_H
#define QBEARERENGINE_P_H

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

#include "qnetworkconfiguration_p.h"
#include "qnetworksession.h"
#include "qnetworkconfigmanager.h"

#include <QtCore/qobject.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qmutex.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QNetworkConfiguration;

class Q_NETWORK_EXPORT QBearerEngine : public QObject
{
    Q_OBJECT

    friend class QNetworkConfigurationManagerPrivate;

public:
    explicit QBearerEngine(QObject *parent = 0);
    virtual ~QBearerEngine();

    virtual bool hasIdentifier(const QString &id) = 0;

    virtual QNetworkConfigurationManager::Capabilities capabilities() const = 0;

    virtual QNetworkSessionPrivate *createSessionBackend() = 0;

    virtual QNetworkConfigurationPrivatePointer defaultConfiguration() = 0;

    virtual bool requiresPolling() const;
    bool configurationsInUse() const;

Q_SIGNALS:
    void configurationAdded(QNetworkConfigurationPrivatePointer config);
    void configurationRemoved(QNetworkConfigurationPrivatePointer config);
    void configurationChanged(QNetworkConfigurationPrivatePointer config);
    void updateCompleted();

protected:
    //this table contains an up to date list of all configs at any time.
    //it must be updated if configurations change, are added/removed or
    //the members of ServiceNetworks change
    QHash<QString, QNetworkConfigurationPrivatePointer> accessPointConfigurations;
    QHash<QString, QNetworkConfigurationPrivatePointer> snapConfigurations;
    QHash<QString, QNetworkConfigurationPrivatePointer> userChoiceConfigurations;

    mutable QMutex mutex;
};

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif // QBEARERENGINE_P_H
