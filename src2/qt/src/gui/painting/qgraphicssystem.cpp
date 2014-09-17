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

#include "qgraphicssystem_p.h"

#ifdef Q_WS_X11
# include <private/qpixmap_x11_p.h>
#endif
#if defined(Q_WS_WIN)
# include <private/qpixmap_raster_p.h>
#endif
#ifdef Q_WS_MAC
# include <private/qpixmap_mac_p.h>
#endif
#ifdef Q_WS_QPA
# include <QtGui/private/qapplication_p.h>
#endif
#ifdef Q_OS_SYMBIAN
# include <private/qpixmap_raster_symbian_p.h>
# include <private/qgraphicssystemex_symbian_p.h>
#else
# include <private/qgraphicssystemex_p.h>
#endif

QT_BEGIN_NAMESPACE

QGraphicsSystem::~QGraphicsSystem()
{
}

QPixmapData *QGraphicsSystem::createDefaultPixmapData(QPixmapData::PixelType type)
{
#ifdef Q_WS_QWS
    Q_UNUSED(type);
#endif
#if defined(Q_WS_X11)
    return new QX11PixmapData(type);
#elif defined(Q_WS_WIN)
    return new QRasterPixmapData(type);
#elif defined(Q_WS_MAC)
    return new QMacPixmapData(type);
#elif defined(Q_WS_QPA)
    return QApplicationPrivate::platformIntegration()->createPixmapData(type);
#elif defined(Q_OS_SYMBIAN)
    return new QSymbianRasterPixmapData(type);    
#elif !defined(Q_WS_QWS)
#error QGraphicsSystem::createDefaultPixmapData() not implemented
#endif
    return 0;
}

QPixmapData *QGraphicsSystem::createPixmapData(QPixmapData *origin)
{
    return createPixmapData(origin->pixelType());
}

#ifdef Q_OS_SYMBIAN
Q_GLOBAL_STATIC(QSymbianGraphicsSystemEx, symbianPlatformExtension)
#endif

QGraphicsSystemEx* QGraphicsSystem::platformExtension()
{
#ifdef Q_OS_SYMBIAN
    // this is used on raster graphics systems. HW accelerated
    // graphics systems will overwrite this function.
    return symbianPlatformExtension();
#endif
    return 0;
}

QT_END_NAMESPACE
