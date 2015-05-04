/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdirectfbintegration.h"
#include "qdirectfbbackingstore.h"
#include "qdirectfbblitter.h"
#include "qdirectfbconvenience.h"
#include "qdirectfbcursor.h"
#include "qdirectfbwindow.h"

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>

#include <QtGui/private/qpixmap_blitter_p.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformpixmap.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QAbstractEventDispatcher>
#include <qpa/qplatforminputcontextfactory_p.h>

QT_BEGIN_NAMESPACE

QDirectFbIntegration::QDirectFbIntegration()
    : m_fontDb(new QGenericUnixFontDatabase())
    , m_services(new QGenericUnixServices)
{
}

void QDirectFbIntegration::connectToDirectFb()
{
    initializeDirectFB();
    initializeScreen();
    initializeInput();

    m_inputContext = QPlatformInputContextFactory::create();
}

bool QDirectFbIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case MultipleWindows: return true;
#ifdef DIRECTFB_GL_EGL
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
#endif
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

void QDirectFbIntegration::initializeDirectFB()
{
    const QStringList args = QCoreApplication::arguments();
    int argc = args.size();
    char **argv = new char*[argc];

    for (int i = 0; i < argc; ++i)
        argv[i] = qstrdup(args.at(i).toLocal8Bit().constData());

    DFBResult result = DirectFBInit(&argc, &argv);
    if (result != DFB_OK) {
        DirectFBError("QDirectFBScreen: error initializing DirectFB",
                      result);
    }

    for (int i = 0; i < argc; ++i)
        delete[] argv[i];
    delete[] argv;

    // This must happen after DirectFBInit.
    m_dfb.reset(QDirectFbConvenience::dfbInterface());
}

void QDirectFbIntegration::initializeScreen()
{
    m_primaryScreen.reset(new QDirectFbScreen(0));
    screenAdded(m_primaryScreen.data());
}

void QDirectFbIntegration::initializeInput()
{
    m_input.reset(new QDirectFbInput(m_dfb.data(), m_primaryScreen->dfbLayer()));
    m_input->start();
}

QDirectFbIntegration::~QDirectFbIntegration()
{
    m_input->stopInputEventLoop();
    m_input->wait();
}

QPlatformPixmap *QDirectFbIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    if (type == QPlatformPixmap::BitmapType)
        return new QRasterPlatformPixmap(type);
    else
        return new QDirectFbBlitterPlatformPixmap;
}

QPlatformWindow *QDirectFbIntegration::createPlatformWindow(QWindow *window) const
{
    QDirectFbWindow *dfbWindow = new QDirectFbWindow(window, m_input.data());
    dfbWindow->createDirectFBWindow();
    return dfbWindow;
}

QAbstractEventDispatcher *QDirectFbIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformBackingStore *QDirectFbIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QDirectFbBackingStore(window);
}

QPlatformFontDatabase *QDirectFbIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformServices *QDirectFbIntegration::services() const
{
    return m_services.data();
}

QPlatformNativeInterface *QDirectFbIntegration::nativeInterface() const
{
    return const_cast<QDirectFbIntegration *>(this);
}

QT_END_NAMESPACE
