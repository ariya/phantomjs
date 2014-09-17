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

#include "qpixmapdatafactory_p.h"

#ifdef Q_WS_QWS
# include <QtGui/qscreen_qws.h>
#endif
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
# include <private/qpixmap_raster_p.h>
#endif
#ifdef Q_OS_SYMBIAN
# include <private/qpixmap_raster_symbian_p.h>
#endif

#include "private/qapplication_p.h"
#include "private/qgraphicssystem_p.h"

QT_BEGIN_NAMESPACE

#if !defined(Q_WS_QWS)

class QSimplePixmapDataFactory : public QPixmapDataFactory
{
public:
    ~QSimplePixmapDataFactory() {}
    QPixmapData* create(QPixmapData::PixelType type);
};

QPixmapData* QSimplePixmapDataFactory::create(QPixmapData::PixelType type)
{
    if (QApplicationPrivate::graphicsSystem())
        return QApplicationPrivate::graphicsSystem()->createPixmapData(type);

#if defined(Q_WS_X11)
    return new QX11PixmapData(type);
#elif defined(Q_WS_WIN)
    return new QRasterPixmapData(type);
#elif defined(Q_WS_MAC)
    return new QMacPixmapData(type);
#elif defined(Q_WS_QPA)
    return new QRasterPixmapData(type);
#elif defined(Q_OS_SYMBIAN)
    return new QSymbianRasterPixmapData(type);    
#else
#error QSimplePixmapDataFactory::create() not implemented
#endif
}

Q_GLOBAL_STATIC(QSimplePixmapDataFactory, factory)

#endif // !defined(Q_WS_QWS)

QPixmapDataFactory::~QPixmapDataFactory()
{
}

QPixmapDataFactory* QPixmapDataFactory::instance(int screen)
{
    Q_UNUSED(screen);
#ifdef Q_WS_QWS
    return QScreen::instance()->pixmapDataFactory();
#else
    return factory();
#endif
}

QT_END_NAMESPACE
