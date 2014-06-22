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

#ifndef QPLATFORMINTEGRATION_UIKIT_H
#define QPLATFORMINTEGRATION_UIKIT_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qwindowsysteminterface.h>

#include "qiosapplicationstate.h"

QT_BEGIN_NAMESPACE

class QIOSServices;

class QIOSIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    QIOSIntegration();
    ~QIOSIntegration();

    bool hasCapability(Capability cap) const;

    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;

    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const Q_DECL_OVERRIDE;

    QPlatformFontDatabase *fontDatabase() const;
    QPlatformClipboard *clipboard() const;
    QPlatformInputContext *inputContext() const;
    QPlatformServices *services() const Q_DECL_OVERRIDE;

    QVariant styleHint(StyleHint hint) const;

    QStringList themeNames() const;
    QPlatformTheme *createPlatformTheme(const QString &name) const;

    QAbstractEventDispatcher *createEventDispatcher() const;
    QPlatformNativeInterface *nativeInterface() const;

    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window);

    QTouchDevice *touchDevice();
private:
    QPlatformFontDatabase *m_fontDatabase;
    QPlatformClipboard *m_clipboard;
    QPlatformInputContext *m_inputContext;
    QPlatformScreen *m_screen;
    QTouchDevice *m_touchDevice;
    QIOSApplicationState m_applicationState;
    QIOSServices *m_platformServices;
};

QT_END_NAMESPACE

#endif

