/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qaccessible.h"
#include "qaccessiblebridge.h"

#ifndef QT_NO_ACCESSIBILITY

#include "qcoreapplication.h"
#include "qmutex.h"
#include "qvector.h"
#include "private/qfactoryloader_p.h"

#include <stdlib.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QAccessibleBridgeFactoryInterface_iid, QLatin1String("/accessiblebridge")))
#endif
Q_GLOBAL_STATIC(QVector<QAccessibleBridge *>, bridges)
static bool isInit = false;

void QAccessible::initialize()
{
    if (isInit)
        return;
    isInit = true;

    if (qgetenv("QT_ACCESSIBILITY") != "1")
        return;
#ifndef QT_NO_LIBRARY
    const QStringList l = loader()->keys();
    for (int i = 0; i < l.count(); ++i) {
        if (QAccessibleBridgeFactoryInterface *factory =
                qobject_cast<QAccessibleBridgeFactoryInterface*>(loader()->instance(l.at(i)))) {
            QAccessibleBridge * bridge = factory->create(l.at(i));
            if (bridge)
                bridges()->append(bridge);
        }
    }
#endif
}

void QAccessible::cleanup()
{
    qDeleteAll(*bridges());
}

void QAccessible::updateAccessibility(QObject *o, int who, Event reason)
{
    Q_ASSERT(o);

    if (updateHandler) {
        updateHandler(o, who, reason);
        return;
    }

    initialize();
    if (!bridges() || bridges()->isEmpty())
        return;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(o);
    if (!iface)
        return;

    // updates for List/Table/Tree should send child
    if (who) {
        QAccessibleInterface *child;
        iface->navigate(QAccessible::Child, who, &child);
        if (child) {
            delete iface;
            iface = child;
            who = 0;
        }
    }

    for (int i = 0; i < bridges()->count(); ++i)
        bridges()->at(i)->notifyAccessibilityUpdate(reason, iface, who);
    delete iface;
}

void QAccessible::setRootObject(QObject *o)
{
    if (rootObjectHandler) {
        rootObjectHandler(o);
        return;
    }

    initialize();
    if (bridges()->isEmpty())
        return;

    if (!o)
        return;

    for (int i = 0; i < bridges()->count(); ++i) {
        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(o);
        bridges()->at(i)->setRootObject(iface);
    }
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY

