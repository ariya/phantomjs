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

#include "qgenericpluginfactory_qpa.h"

#include "qapplication.h"
#include "private/qfactoryloader_p.h"
#include "qgenericplugin_qpa.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QGenericPluginFactoryInterface_iid,
     QLatin1String("/generic"), Qt::CaseInsensitive))

#endif //QT_NO_LIBRARY
#endif //QT_MAKEDLL

/*!
    \class QGenericPluginFactory
    \ingroup qpa
    \since 4.8

    \brief The QGenericPluginFactory class creates window-system
    related plugin drivers in Qt QPA.

    Note that this class is only available in Qt QPA.

    \sa QGenericPlugin
*/

/*!
    Creates the driver specified by \a key, using the given \a specification.

    Note that the keys are case-insensitive.

    \sa keys()
*/
QObject *QGenericPluginFactory::create(const QString& key, const QString &specification)
{
    QString driver = key.toLower();

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY
    if (QGenericPluginFactoryInterface *factory = qobject_cast<QGenericPluginFactoryInterface*>(loader()->instance(driver)))
        return factory->create(driver, specification);
#endif
#endif
    return 0;
}

/*!
    Returns the list of valid keys, i.e. the available mouse drivers.

    \sa create()
*/
QStringList QGenericPluginFactory::keys()
{
    QStringList list;

#if !defined(Q_OS_WIN32) || defined(QT_MAKEDLL)
#ifndef QT_NO_LIBRARY
    QStringList plugins = loader()->keys();
    for (int i = 0; i < plugins.size(); ++i) {
        if (!list.contains(plugins.at(i)))
            list += plugins.at(i);
    }
#endif //QT_NO_LIBRARY
#endif //QT_MAKEDLL
    return list;
}

QT_END_NAMESPACE
