/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QACCESSIBLEBRIDGE_H
#define QACCESSIBLEBRIDGE_H

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ACCESSIBILITY

class QAccessibleInterface;

class QAccessibleBridge
{
public:
    virtual ~QAccessibleBridge() {}
    virtual void setRootObject(QAccessibleInterface *) = 0;
    virtual void notifyAccessibilityUpdate(int, QAccessibleInterface*, int) = 0;
};

struct Q_GUI_EXPORT QAccessibleBridgeFactoryInterface : public QFactoryInterface
{
    virtual QAccessibleBridge *create(const QString& name) = 0;
};

#define QAccessibleBridgeFactoryInterface_iid "com.trolltech.Qt.QAccessibleBridgeFactoryInterface"
Q_DECLARE_INTERFACE(QAccessibleBridgeFactoryInterface, QAccessibleBridgeFactoryInterface_iid)

class Q_GUI_EXPORT QAccessibleBridgePlugin : public QObject, public QAccessibleBridgeFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QAccessibleBridgeFactoryInterface:QFactoryInterface)
public:
    explicit QAccessibleBridgePlugin(QObject *parent = 0);
    ~QAccessibleBridgePlugin();

    virtual QStringList keys() const = 0;
    virtual QAccessibleBridge *create(const QString &key) = 0;
};

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

QT_END_HEADER

#endif // QACCESSIBLEBRIDGE_H
