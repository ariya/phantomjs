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

#ifndef QXCBNATIVEINTERFACE_H
#define QXCBNATIVEINTERFACE_H

#include <qpa/qplatformnativeinterface.h>
#include <xcb/xcb.h>

#include <QtCore/QRect>

QT_BEGIN_NAMESPACE

class QWidget;
class QXcbScreen;
class QXcbConnection;

class QXcbNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT
public:
    enum ResourceType {
        Display,
        EglDisplay,
        Connection,
        Screen,
        EglContext,
        EglConfig,
        GLXConfig,
        GLXContext,
        AppTime,
        AppUserTime,
        ScreenHintStyle,
        StartupId,
        TrayWindow,
        GetTimestamp,
        X11Screen,
        RootWindow,
        ScreenSubpixelType,
        ScreenAntialiasingEnabled,
        NoFontHinting
    };

    QXcbNativeInterface();

    void *nativeResourceForIntegration(const QByteArray &resource) Q_DECL_OVERRIDE;
    void *nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context) Q_DECL_OVERRIDE;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) Q_DECL_OVERRIDE;
    void *nativeResourceForWindow(const QByteArray &resourceString, QWindow *window) Q_DECL_OVERRIDE;

    NativeResourceForIntegrationFunction nativeResourceFunctionForIntegration(const QByteArray &resource) Q_DECL_OVERRIDE;
    NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource) Q_DECL_OVERRIDE;
    NativeResourceForScreenFunction nativeResourceFunctionForScreen(const QByteArray &resource) Q_DECL_OVERRIDE;

    QFunctionPointer platformFunction(const QByteArray &function) const Q_DECL_OVERRIDE;

    inline const QByteArray &genericEventFilterType() const { return m_genericEventFilterType; }

    void *displayForWindow(QWindow *window);
    void *eglDisplayForWindow(QWindow *window);
    void *connectionForWindow(QWindow *window);
    void *screenForWindow(QWindow *window);
    void *appTime(const QXcbScreen *screen);
    void *appUserTime(const QXcbScreen *screen);
    void *getTimestamp(const QXcbScreen *screen);
    void *startupId();
    void *x11Screen();
    void *rootWindow();
    static void setStartupId(const char *);
    static void setAppTime(QScreen *screen, xcb_timestamp_t time);
    static void setAppUserTime(QScreen *screen, xcb_timestamp_t time);
    static void *eglContextForContext(QOpenGLContext *context);
    static void *eglConfigForContext(QOpenGLContext *context);
    static void *glxContextForContext(QOpenGLContext *context);
    static void *glxConfigForContext(QOpenGLContext *context);

    Q_INVOKABLE void beep();
    Q_INVOKABLE bool systemTrayAvailable(const QScreen *screen) const;
    Q_INVOKABLE void clearRegion(const QWindow *qwindow, const QRect& rect);
    Q_INVOKABLE bool systrayVisualHasAlphaChannel();
    Q_INVOKABLE bool requestSystemTrayWindowDock(const QWindow *window);
    Q_INVOKABLE QRect systemTrayWindowGlobalGeometry(const QWindow *window);

signals:
    void systemTrayWindowChanged(QScreen *screen);

private:
    xcb_window_t locateSystemTray(xcb_connection_t *conn, const QXcbScreen *screen);

    const QByteArray m_genericEventFilterType;

    xcb_atom_t m_sysTraySelectionAtom;
    xcb_visualid_t m_systrayVisualId;

    static QXcbScreen *qPlatformScreenForWindow(QWindow *window);
};

QT_END_NAMESPACE

#endif // QXCBNATIVEINTERFACE_H
