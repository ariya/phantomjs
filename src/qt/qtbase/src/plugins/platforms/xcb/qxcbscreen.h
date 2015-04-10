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

#ifndef QXCBSCREEN_H
#define QXCBSCREEN_H

#include <qpa/qplatformscreen.h>
#include <QtCore/QString>

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include "qxcbobject.h"

#include <private/qfontengine_p.h>

QT_BEGIN_NAMESPACE

class QXcbConnection;
class QXcbCursor;
class QXcbXSettings;

class QXcbScreen : public QXcbObject, public QPlatformScreen
{
public:
    QXcbScreen(QXcbConnection *connection, xcb_screen_t *screen,
               xcb_randr_get_output_info_reply_t *output, QString outputName, int number);
    ~QXcbScreen();

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const;

    QWindow *topLevelAt(const QPoint &point) const;

    QRect geometry() const { return m_geometry; }
    QRect availableGeometry() const {return m_availableGeometry;}
    int depth() const { return m_screen->root_depth; }
    QImage::Format format() const;
    QSizeF physicalSize() const { return m_sizeMillimeters; }
    QDpi logicalDpi() const;
    QPlatformCursor *cursor() const;
    qreal refreshRate() const { return m_refreshRate; }
    Qt::ScreenOrientation orientation() const { return m_orientation; }
    QList<QPlatformScreen *> virtualSiblings() const { return m_siblings; }
    void setVirtualSiblings(QList<QPlatformScreen *> sl) { m_siblings = sl; }

    int screenNumber() const { return m_number; }

    xcb_screen_t *screen() const { return m_screen; }
    xcb_window_t root() const { return m_screen->root; }

    xcb_window_t clientLeader() const { return m_clientLeader; }

    void windowShown(QXcbWindow *window);
    QString windowManagerName() const { return m_windowManagerName; }
    bool syncRequestSupported() const { return m_syncRequestSupported; }

    const xcb_visualtype_t *visualForId(xcb_visualid_t) const;

    QString name() const { return m_outputName; }

    void handleScreenChange(xcb_randr_screen_change_notify_event_t *change_event);
    void updateGeometry(xcb_timestamp_t timestamp);
    void updateRefreshRate();

    void readXResources();

    QFontEngine::HintStyle hintStyle() const { return m_hintStyle; }

    QXcbXSettings *xSettings() const;

private:
    static bool xResource(const QByteArray &identifier,
                         const QByteArray &expectedIdentifier,
                         int *value);
    void sendStartupMessage(const QByteArray &message) const;

    xcb_screen_t *m_screen;
    xcb_randr_crtc_t m_crtc;
    QString m_outputName;
    QSizeF m_sizeMillimeters;
    QRect m_geometry;
    QRect m_availableGeometry;
    QSize m_virtualSize;
    QSizeF m_virtualSizeMillimeters;
    QList<QPlatformScreen *> m_siblings;
    Qt::ScreenOrientation m_orientation;
    int m_number;
    QString m_windowManagerName;
    bool m_syncRequestSupported;
    xcb_window_t m_clientLeader;
    QMap<xcb_visualid_t, xcb_visualtype_t> m_visuals;
    QXcbCursor *m_cursor;
    int m_refreshRate;
    int m_forcedDpi;
    QFontEngine::HintStyle m_hintStyle;
    QXcbXSettings *m_xSettings;
};

QT_END_NAMESPACE

#endif
