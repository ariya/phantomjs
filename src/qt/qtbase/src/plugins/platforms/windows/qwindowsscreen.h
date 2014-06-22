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

#ifndef QWINDOWSSCREEN_H
#define QWINDOWSSCREEN_H

#include "qwindowscursor.h"
#ifdef Q_OS_WINCE
#  include "qplatformfunctions_wince.h"
#endif

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

struct QWindowsScreenData
{
    enum Flags
    {
        PrimaryScreen = 0x1,
        VirtualDesktop = 0x2,
        LockScreen = 0x4 // Temporary screen existing during user change, etc.
    };

    QWindowsScreenData();

    QRect geometry;
    QRect availableGeometry;
    QDpi dpi;
    QSizeF physicalSizeMM;
    int depth;
    QImage::Format format;
    unsigned flags;
    QString name;
    Qt::ScreenOrientation orientation;
    qreal refreshRateHz;
};

class QWindowsScreen : public QPlatformScreen
{
public:
#ifndef QT_NO_CURSOR
    typedef QSharedPointer<QWindowsCursor> WindowsCursorPtr;
#endif

    explicit QWindowsScreen(const QWindowsScreenData &data);

    static QWindowsScreen *screenOf(const QWindow *w = 0);

    virtual QRect geometry() const { return m_data.geometry; }
    virtual QRect availableGeometry() const { return m_data.availableGeometry; }
    virtual int depth() const { return m_data.depth; }
    virtual QImage::Format format() const { return m_data.format; }
    virtual QSizeF physicalSize() const { return m_data.physicalSizeMM; }
    virtual QDpi logicalDpi() const { return m_data.dpi; }
    virtual qreal refreshRate() const { return m_data.refreshRateHz; }
    virtual QString name() const { return m_data.name; }
    virtual Qt::ScreenOrientation primaryOrientation() { return m_data.orientation; }
    virtual QList<QPlatformScreen *> virtualSiblings() const;
    virtual QWindow *topLevelAt(const QPoint &point) const
        {  return QWindowsScreen::findTopLevelAt(point, CWP_SKIPINVISIBLE);  }

    static QWindow *findTopLevelAt(const QPoint &point, unsigned flags);
    static QWindow *windowAt(const QPoint &point, unsigned flags = CWP_SKIPINVISIBLE);
    static QWindow *windowUnderMouse(unsigned flags = CWP_SKIPINVISIBLE);

    virtual QPixmap grabWindow(WId window, int x, int y, int width, int height) const;

    inline void handleChanges(const QWindowsScreenData &newData);

#ifndef QT_NO_CURSOR
    QPlatformCursor *cursor() const               { return m_cursor.data(); }
    const WindowsCursorPtr &windowsCursor() const { return m_cursor; }
#else
    QPlatformCursor *cursor() const               { return 0; }
#endif // !QT_NO_CURSOR

    const QWindowsScreenData &data() const  { return m_data; }

private:
    QWindowsScreenData m_data;
#ifndef QT_NO_CURSOR
    const WindowsCursorPtr m_cursor;
#endif
};

class QWindowsScreenManager
{
public:
    typedef QList<QWindowsScreen *> WindowsScreenList;

    QWindowsScreenManager();

    inline void clearScreens() {
        // Delete screens in reverse order to avoid crash in case of multiple screens
        while (!m_screens.isEmpty())
            delete m_screens.takeLast();
    }

    bool handleScreenChanges();
    bool handleDisplayChange(WPARAM wParam, LPARAM lParam);
    const WindowsScreenList &screens() const { return m_screens; }

private:
    WindowsScreenList m_screens;
    int m_lastDepth;
    WORD m_lastHorizontalResolution;
    WORD m_lastVerticalResolution;
};

QT_END_NAMESPACE

#endif // QWINDOWSSCREEN_H
