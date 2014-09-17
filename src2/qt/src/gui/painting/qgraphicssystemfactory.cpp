/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgraphicssystemfactory_p.h"
#include "qgraphicssystemplugin_p.h"
#include "private/qfactoryloader_p.h"
#include "qmutex.h"

#include "qapplication.h"
#include "qgraphicssystem_raster_p.h"
#include "qgraphicssystem_runtime_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QGraphicsSystemFactoryInterface_iid, QLatin1String("/graphicssystems"), Qt::CaseInsensitive))
#endif

QGraphicsSystem *QGraphicsSystemFactory::create(const QString& key)
{
    QGraphicsSystem *ret = 0;
    QString system = key.toLower();

#if defined (QT_GRAPHICSSYSTEM_OPENGL)
    if (system.isEmpty()) {
        system = QLatin1String("opengl");
    }
#elif defined (QT_GRAPHICSSYSTEM_OPENVG) && !defined(Q_CC_NOKIAX86)
    if (system.isEmpty()) {
        system = QLatin1String("openvg");
    }
#elif defined (QT_GRAPHICSSYSTEM_RUNTIME)
    if (system.isEmpty()) {
        system = QLatin1String("runtime");
    }
#elif defined (QT_GRAPHICSSYSTEM_RASTER) && !defined(Q_WS_WIN) && !defined(Q_OS_SYMBIAN) || defined(Q_WS_X11)
    if (system.isEmpty()) {
        system = QLatin1String("raster");
    }
#endif

    if (system == QLatin1String("raster"))
        return new QRasterGraphicsSystem;
    else if (system == QLatin1String("runtime"))
        return new QRuntimeGraphicsSystem;
    else if (system.isEmpty() || system == QLatin1String("native"))
        return 0;

#ifndef QT_NO_LIBRARY
    if (!ret) {
        if (QGraphicsSystemFactoryInterface *factory = qobject_cast<QGraphicsSystemFactoryInterface*>(loader()->instance(system)))
            ret = factory->create(system);
    }
#endif

    if (!ret)
        qWarning() << "Unable to load graphicssystem" << system;

    return ret;
}

/*!
    Returns the list of valid keys, i.e. the keys this factory can
    create styles for.

    \sa create()
*/
QStringList QGraphicsSystemFactory::keys()
{
#ifndef QT_NO_LIBRARY
    QStringList list = loader()->keys();
#else
    QStringList list;
#endif
    if (!list.contains(QLatin1String("Raster")))
        list << QLatin1String("raster");
    return list;
}

QT_END_NAMESPACE

