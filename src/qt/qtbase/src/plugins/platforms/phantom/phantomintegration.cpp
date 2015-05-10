/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "phantomintegration.h"
#include "phantombackingstore.h"

#include <private/qpixmap_raster_p.h>

#if defined(Q_OS_MAC)
# include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>
#else
# include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#endif

#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>

PhantomIntegration::PhantomIntegration()
{
    PhantomScreen *mPrimaryScreen = new PhantomScreen();

    // Simulate typical desktop screen
    int width = 1024;
    int height = 768;
    int dpi = 72;
    qreal physicalWidth = width * 25.4 / dpi;
    qreal physicalHeight = height * 25.4 / dpi;
    mPrimaryScreen->mGeometry = QRect(0, 0, width, height);
    mPrimaryScreen->mPhysicalSize = QSizeF(physicalWidth, physicalHeight);

    mPrimaryScreen->mDepth = 32;
    mPrimaryScreen->mFormat = QImage::Format_ARGB32_Premultiplied;

    screenAdded(mPrimaryScreen);

    m_phantomPlatformNativeInterface = new PhantomPlatformNativeInterface();
}

PhantomIntegration::~PhantomIntegration()
{
    delete m_phantomPlatformNativeInterface;
}

bool PhantomIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow* PhantomIntegration::createPlatformWindow(QWindow* window) const
{
    return new QPlatformWindow(window);
}

QPlatformBackingStore* PhantomIntegration::createPlatformBackingStore(QWindow* window) const
{
    return new PhantomBackingStore(window);
}

QPlatformFontDatabase *PhantomIntegration::fontDatabase() const
{
    static QPlatformFontDatabase *db = 0;
    if (!db) {
#if defined(Q_OS_MAC)
        db = new QCoreTextFontDatabase();
#else
        db = new QGenericUnixFontDatabase();
#endif
    }
    return db;
}

QAbstractEventDispatcher *PhantomIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformNativeInterface *PhantomIntegration::nativeInterface() const
{
    return m_phantomPlatformNativeInterface;
}
