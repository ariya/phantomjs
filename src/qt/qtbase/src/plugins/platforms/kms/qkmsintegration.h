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

#ifndef QPLATFORMINTEGRATION_KMS_H
#define QPLATFORMINTEGRATION_KMS_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformoffscreensurface.h>
#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QKmsScreen;
class QKmsDevice;
class QFbVtHandler;

class QKmsOffscreenWindow : public QPlatformOffscreenSurface
{
public:
    QKmsOffscreenWindow(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface);
    ~QKmsOffscreenWindow();

    QSurfaceFormat format() const Q_DECL_OVERRIDE { return m_format; }
    bool isValid() const Q_DECL_OVERRIDE { return m_surface != EGL_NO_SURFACE; }

    EGLSurface surface() const { return m_surface; }

private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLNativeWindowType m_window;
};

class QKmsIntegration : public QObject, public QPlatformIntegration
{
    Q_OBJECT

public:
    QKmsIntegration();
    ~QKmsIntegration();

    void initialize() Q_DECL_OVERRIDE;
    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;

    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const Q_DECL_OVERRIDE;
    QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const Q_DECL_OVERRIDE;

    QPlatformFontDatabase *fontDatabase() const Q_DECL_OVERRIDE;
    QAbstractEventDispatcher *createEventDispatcher() const Q_DECL_OVERRIDE;

    QPlatformNativeInterface *nativeInterface() const Q_DECL_OVERRIDE;

    void addScreen(QKmsScreen *screen);
    QObject *createDevice(const char *);

private slots:
    void addDevice(const QString &deviceNode);
    void removeDevice(const QString &deviceNode);

private:
    QStringList findDrmDevices();

    QList<QPlatformScreen *> m_screens;
    QList<QKmsDevice *> m_devices;
    QPlatformFontDatabase *m_fontDatabase;
    QPlatformNativeInterface *m_nativeInterface;
    QFbVtHandler *m_vtHandler;
    QDeviceDiscovery *m_deviceDiscovery;
};

QT_END_NAMESPACE

#endif
