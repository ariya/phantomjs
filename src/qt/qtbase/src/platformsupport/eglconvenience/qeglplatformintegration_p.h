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

#ifndef QEGLPLATFORMINTEGRATION_H
#define QEGLPLATFORMINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QEGLPlatformScreen;
class QEGLPlatformWindow;
class QEGLPlatformContext;
class QFbVtHandler;

class QEGLPlatformIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    QEGLPlatformIntegration();
    ~QEGLPlatformIntegration();

    void initialize() Q_DECL_OVERRIDE;

    QEGLPlatformScreen *screen() const { return m_screen; }
    EGLDisplay display() const { return m_display; }

    QAbstractEventDispatcher *createEventDispatcher() const Q_DECL_OVERRIDE;
    QPlatformFontDatabase *fontDatabase() const Q_DECL_OVERRIDE;
    QPlatformServices *services() const Q_DECL_OVERRIDE;
    QPlatformInputContext *inputContext() const Q_DECL_OVERRIDE { return m_inputContext; }

    QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const Q_DECL_OVERRIDE;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const Q_DECL_OVERRIDE;

    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;

    QPlatformNativeInterface *nativeInterface() const Q_DECL_OVERRIDE;
    // QPlatformNativeInterface
    void *nativeResourceForIntegration(const QByteArray &resource) Q_DECL_OVERRIDE;
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) Q_DECL_OVERRIDE;
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) Q_DECL_OVERRIDE;
    NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource) Q_DECL_OVERRIDE;

protected:
    virtual QEGLPlatformScreen *createScreen() const = 0;
    virtual QEGLPlatformWindow *createWindow(QWindow *window) const = 0;
    virtual QEGLPlatformContext *createContext(const QSurfaceFormat &format,
                                               QPlatformOpenGLContext *shareContext,
                                               EGLDisplay display) const = 0;
    virtual QPlatformOffscreenSurface *createOffscreenSurface(EGLDisplay display,
                                                              const QSurfaceFormat &format,
                                                              QOffscreenSurface *surface) const = 0;

    virtual EGLNativeDisplayType nativeDisplay() const { return EGL_DEFAULT_DISPLAY; }

    void createInputHandlers();

private:
    QEGLPlatformScreen *m_screen;
    EGLDisplay m_display;
    QPlatformInputContext *m_inputContext;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    QScopedPointer<QPlatformServices> m_services;
    QScopedPointer<QFbVtHandler> m_vtHandler;
};

QT_END_NAMESPACE

#endif // QEGLPLATFORMINTEGRATION_H
