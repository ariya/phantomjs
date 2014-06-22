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
//#include <QDebug>
#include "qkmsscreen.h"
#include "qkmsdevice.h"

#include "qkmsintegration.h"

#include <QtCore/QSocketNotifier>
#include <QtCore/private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

QKmsDevice::QKmsDevice(const QString &path, QKmsIntegration *parent) :
    QObject(0), m_integration(parent)
{
    m_fd = QT_OPEN(path.toLatin1().constData(), O_RDWR);
    if (m_fd < 0) {
        qWarning("Could not open %s.", path.toLatin1().constData());
        qFatal("No DRM display device");
    }

    m_graphicsBufferManager = gbm_create_device(m_fd);
    m_eglDisplay = eglGetDisplay(m_graphicsBufferManager);

    if (m_eglDisplay == EGL_NO_DISPLAY) {
        qWarning("Could not open EGL display");
        qFatal("EGL error");
    }

    EGLint major;
    EGLint minor;
    if (!eglInitialize(m_eglDisplay, &major, &minor)) {
        qWarning("Could not initialize EGL display");
        qFatal("EGL error");
    }

    createScreens();

//    QSocketNotifier *notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
//    connect(notifier, SIGNAL(activated(int)), this, SLOT(handlePageFlipCompleted()));
}

QKmsDevice::~QKmsDevice()
{
}

void QKmsDevice::createScreens()
{
    drmModeRes *resources = 0;
    resources = drmModeGetResources(m_fd);
    if (!resources)
        qFatal("drmModeGetResources failed");

    //Iterate connectors and create screens on each one active
    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector *connector = 0;
        connector = drmModeGetConnector(m_fd, resources->connectors[i]);
        if (connector && connector->connection == DRM_MODE_CONNECTED) {
            m_integration->addScreen(new QKmsScreen(this, connector->connector_id));
        }
        drmModeFreeConnector(connector);
    }
    drmModeFreeResources(resources);
}

void QKmsDevice::handlePageFlipCompleted()
{
    drmEventContext eventContext;

    memset(&eventContext, 0, sizeof eventContext);
    eventContext.version = DRM_EVENT_CONTEXT_VERSION;
    eventContext.page_flip_handler = QKmsDevice::pageFlipHandler;
    drmHandleEvent(m_fd, &eventContext);

}

void QKmsDevice::pageFlipHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
    Q_UNUSED(fd)
    Q_UNUSED(frame)
    Q_UNUSED(sec)
    Q_UNUSED(usec)

    QKmsScreen *screen = static_cast<QKmsScreen *>(data);
    screen->handlePageFlipped();
}

QT_END_NAMESPACE
