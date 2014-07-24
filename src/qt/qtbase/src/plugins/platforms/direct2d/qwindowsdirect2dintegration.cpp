/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dintegration.h"
#include "qwindowsdirect2dbackingstore.h"
#include "qwindowsdirect2dplatformpixmap.h"
#include "qwindowsdirect2dnativeinterface.h"
#include "qwindowsdirect2dwindow.h"

#include "qwindowscontext.h"

#include <QtCore/QDebug>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DIntegrationPrivate
{
public:
    QWindowsDirect2DNativeInterface m_nativeInterface;
    QWindowsDirect2DContext m_d2dContext;
};

QWindowsDirect2DIntegration *QWindowsDirect2DIntegration::create(const QStringList &paramList)
{
    QWindowsDirect2DIntegration *integration = new QWindowsDirect2DIntegration(paramList);

    if (!integration->init()) {
        delete integration;
        integration = 0;
    }

    return integration;
}

QWindowsDirect2DIntegration::~QWindowsDirect2DIntegration()
{

}

 QWindowsDirect2DIntegration *QWindowsDirect2DIntegration::instance()
 {
     return static_cast<QWindowsDirect2DIntegration *>(QWindowsIntegration::instance());
 }

 QPlatformWindow *QWindowsDirect2DIntegration::createPlatformWindow(QWindow *window) const
 {
     QWindowsWindowData data = createWindowData(window);
     return data.hwnd ? new QWindowsDirect2DWindow(window, data)
                      : Q_NULLPTR;
 }

 QPlatformNativeInterface *QWindowsDirect2DIntegration::nativeInterface() const
 {
     return &d->m_nativeInterface;
 }

QPlatformPixmap *QWindowsDirect2DIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    switch (type) {
    case QPlatformPixmap::BitmapType:
        return new QRasterPlatformPixmap(type);
        break;
    default:
        return new QWindowsDirect2DPlatformPixmap(type);
        break;
    }
}

QPlatformBackingStore *QWindowsDirect2DIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QWindowsDirect2DBackingStore(window);
}

QWindowsDirect2DContext *QWindowsDirect2DIntegration::direct2DContext() const
{
    return &d->m_d2dContext;
}

QWindowsDirect2DIntegration::QWindowsDirect2DIntegration(const QStringList &paramList)
    : QWindowsIntegration(paramList)
    , d(new QWindowsDirect2DIntegrationPrivate)
{
}

bool QWindowsDirect2DIntegration::init()
{
    return d->m_d2dContext.init();
}

QT_END_NAMESPACE
