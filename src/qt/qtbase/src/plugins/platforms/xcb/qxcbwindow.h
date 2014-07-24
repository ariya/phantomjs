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

#ifndef QXCBWINDOW_H
#define QXCBWINDOW_H

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QImage>

#include <xcb/xcb.h>
#include <xcb/sync.h>

#include "qxcbobject.h"

QT_BEGIN_NAMESPACE

class QXcbScreen;
class QXcbEGLSurface;
class QIcon;
class QXcbWindow : public QXcbObject, public QXcbWindowEventListener, public QPlatformWindow
{
public:
    enum NetWmState {
        NetWmStateAbove = 0x1,
        NetWmStateBelow = 0x2,
        NetWmStateFullScreen = 0x4,
        NetWmStateMaximizedHorz = 0x8,
        NetWmStateMaximizedVert = 0x10,
        NetWmStateModal = 0x20,
        NetWmStateStaysOnTop = 0x40,
        NetWmStateDemandsAttention = 0x80
    };

    Q_DECLARE_FLAGS(NetWmStates, NetWmState)

    QXcbWindow(QWindow *window);
    ~QXcbWindow();

    void setGeometry(const QRect &rect);

    QMargins frameMargins() const;

    void setVisible(bool visible);
    void setWindowFlags(Qt::WindowFlags flags);
    void setWindowState(Qt::WindowState state);
    WId winId() const;
    void setParent(const QPlatformWindow *window);

    bool isExposed() const;
    bool isEmbedded(const QPlatformWindow *parentWindow) const;
    QPoint mapToGlobal(const QPoint &pos) const;
    QPoint mapFromGlobal(const QPoint &pos) const;

    void setWindowTitle(const QString &title);
    void setWindowIcon(const QIcon &icon);
    void raise();
    void lower();
    void propagateSizeHints();

    void requestActivateWindow();

#if XCB_USE_MAEMO_WINDOW_PROPERTIES
    void handleContentOrientationChange(Qt::ScreenOrientation orientation);
#endif

    bool setKeyboardGrabEnabled(bool grab);
    bool setMouseGrabEnabled(bool grab);

    void setCursor(xcb_cursor_t cursor);

    QSurfaceFormat format() const;

    void windowEvent(QEvent *event);

    bool startSystemResize(const QPoint &pos, Qt::Corner corner);

    void setOpacity(qreal level);
    void setMask(const QRegion &region);

    void setAlertState(bool enabled);
    bool isAlertState() const { return m_alertState; }

    xcb_window_t xcb_window() const { return m_window; }
    uint depth() const { return m_depth; }
    QImage::Format imageFormat() const { return m_imageFormat; }

    bool handleGenericEvent(xcb_generic_event_t *event, long *result)  Q_DECL_OVERRIDE;

    void handleExposeEvent(const xcb_expose_event_t *event) Q_DECL_OVERRIDE;
    void handleClientMessageEvent(const xcb_client_message_event_t *event) Q_DECL_OVERRIDE;
    void handleConfigureNotifyEvent(const xcb_configure_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleMapNotifyEvent(const xcb_map_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleUnmapNotifyEvent(const xcb_unmap_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleButtonPressEvent(const xcb_button_press_event_t *event) Q_DECL_OVERRIDE;
    void handleButtonReleaseEvent(const xcb_button_release_event_t *event) Q_DECL_OVERRIDE;
    void handleMotionNotifyEvent(const xcb_motion_notify_event_t *event) Q_DECL_OVERRIDE;

    void handleEnterNotifyEvent(const xcb_enter_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleLeaveNotifyEvent(const xcb_leave_notify_event_t *event) Q_DECL_OVERRIDE;
    void handleFocusInEvent(const xcb_focus_in_event_t *event) Q_DECL_OVERRIDE;
    void handleFocusOutEvent(const xcb_focus_out_event_t *event) Q_DECL_OVERRIDE;
    void handlePropertyNotifyEvent(const xcb_property_notify_event_t *event) Q_DECL_OVERRIDE;

    QXcbWindow *toWindow() Q_DECL_OVERRIDE;

    void handleMouseEvent(xcb_timestamp_t time, const QPoint &local, const QPoint &global, Qt::KeyboardModifiers modifiers);

    void updateSyncRequestCounter();
    void updateNetWmUserTime(xcb_timestamp_t timestamp);

#if defined(XCB_USE_EGL)
    QXcbEGLSurface *eglSurface() const;
#endif

private:
    void changeNetWmState(bool set, xcb_atom_t one, xcb_atom_t two = 0);
    NetWmStates netWmStates();
    void setNetWmStates(NetWmStates);

    void setNetWmWindowFlags(Qt::WindowFlags flags);
    void setMotifWindowFlags(Qt::WindowFlags flags);

    void updateMotifWmHintsBeforeMap();
    void updateNetWmStateBeforeMap();

    void setTransparentForMouseEvents(bool transparent);
    void updateDoesNotAcceptFocus(bool doesNotAcceptFocus);

    QRect windowToWmGeometry(QRect r) const;
    void sendXEmbedMessage(xcb_window_t window, quint32 message,
                           quint32 detail = 0, quint32 data1 = 0, quint32 data2 = 0);
    void handleXEmbedMessage(const xcb_client_message_event_t *event);

    void create();
    void destroy();

    void show();
    void hide();

    QXcbScreen *m_screen;

    xcb_window_t m_window;

    uint m_depth;
    QImage::Format m_imageFormat;

    xcb_sync_int64_t m_syncValue;
    xcb_sync_counter_t m_syncCounter;

    Qt::WindowState m_windowState;

    xcb_gravity_t m_gravity;

    bool m_mapped;
    bool m_transparent;
    bool m_usingSyncProtocol;
    bool m_deferredActivation;
    bool m_deferredExpose;
    bool m_configureNotifyPending;
    bool m_embedded;
    bool m_alertState;
    xcb_window_t m_netWmUserTimeWindow;

    QSurfaceFormat m_format;

    mutable bool m_dirtyFrameMargins;
    mutable QMargins m_frameMargins;

#if defined(XCB_USE_EGL)
    mutable QXcbEGLSurface *m_eglSurface;
#endif

    QRegion m_exposeRegion;

    xcb_visualid_t m_visualId;
    int m_lastWindowStateEvent;
};

QT_END_NAMESPACE

#endif
