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

#ifndef QEGLFSINTEGRATION_H
#define QEGLFSINTEGRATION_H

#include <QtPlatformSupport/private/qeglplatformintegration_p.h>
#include <qpa/qplatformscreen.h>
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QEglFSIntegration : public QEGLPlatformIntegration
{
public:
    QEglFSIntegration();
    ~QEglFSIntegration();

    void initialize() Q_DECL_OVERRIDE;

    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    QVariant styleHint(QPlatformIntegration::StyleHint hint) const Q_DECL_OVERRIDE;

    static EGLConfig chooseConfig(EGLDisplay display, const QSurfaceFormat &format);

protected:
    QEGLPlatformScreen *createScreen() const Q_DECL_OVERRIDE;
    QEGLPlatformWindow *createWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QEGLPlatformContext *createContext(const QSurfaceFormat &format,
                                       QPlatformOpenGLContext *shareContext,
                                       EGLDisplay display,
                                       QVariant *nativeHandle) const Q_DECL_OVERRIDE;
    QPlatformOffscreenSurface *createOffscreenSurface(EGLDisplay display,
                                                      const QSurfaceFormat &format,
                                                      QOffscreenSurface *surface) const Q_DECL_OVERRIDE;
    EGLNativeDisplayType nativeDisplay() const Q_DECL_OVERRIDE;

private:
    bool mDisableInputHandlers;
};

QT_END_NAMESPACE

#endif // QEGLFSINTEGRATION_H
