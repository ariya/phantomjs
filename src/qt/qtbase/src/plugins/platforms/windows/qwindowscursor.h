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

#ifndef QWINDOWSCURSOR_H
#define QWINDOWSCURSOR_H

#include "qtwindows_additional.h"

#include <qpa/qplatformcursor.h>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE

class QWindowsWindowCursorData;

struct QWindowsCursorCacheKey
{
    explicit QWindowsCursorCacheKey(const QCursor &c);
    explicit QWindowsCursorCacheKey(Qt::CursorShape s) : shape(s), bitmapCacheKey(0), maskCacheKey(0) {}
    QWindowsCursorCacheKey() : shape(Qt::CustomCursor), bitmapCacheKey(0), maskCacheKey(0) {}

    Qt::CursorShape shape;
    qint64 bitmapCacheKey;
    qint64 maskCacheKey;
};

inline bool operator==(const QWindowsCursorCacheKey &k1, const QWindowsCursorCacheKey &k2)
{
    return k1.shape == k2.shape && k1.bitmapCacheKey == k2.bitmapCacheKey && k1.maskCacheKey == k2.maskCacheKey;
}

inline uint qHash(const QWindowsCursorCacheKey &k, uint seed) Q_DECL_NOTHROW
{
    return (uint(k.shape) + uint(k.bitmapCacheKey) + uint(k.maskCacheKey)) ^ seed;
}

class QWindowsWindowCursor
{
public:
    QWindowsWindowCursor();
    explicit QWindowsWindowCursor(const QCursor &c);
    ~QWindowsWindowCursor();
    QWindowsWindowCursor(const QWindowsWindowCursor &c);
    QWindowsWindowCursor &operator=(const QWindowsWindowCursor &c);

    bool isNull() const;
    QCursor cursor() const;
    HCURSOR handle() const;

private:
    QSharedDataPointer<QWindowsWindowCursorData> m_data;
};

class QWindowsCursor : public QPlatformCursor
{
public:
    enum CursorState {
        CursorShowing,
        CursorHidden,
        CursorSuppressed // Cursor suppressed by touch interaction (Windows 8).
    };

    QWindowsCursor() {}

    virtual void changeCursor(QCursor * widgetCursor, QWindow * widget);
    virtual QPoint pos() const { return mousePosition(); }
    virtual void setPos(const QPoint &pos);

    static HCURSOR createPixmapCursor(const QPixmap &pixmap, int hotX, int hotY);
    static HCURSOR createSystemCursor(const QCursor &c);
    static QPoint mousePosition();
    static CursorState cursorState();

    QWindowsWindowCursor standardWindowCursor(Qt::CursorShape s = Qt::ArrowCursor);
    QWindowsWindowCursor pixmapWindowCursor(const QCursor &c);

private:
    typedef QHash<QWindowsCursorCacheKey, QWindowsWindowCursor>  CursorCache;

    CursorCache m_cursorCache;
};

QT_END_NAMESPACE

#endif // QWINDOWSCURSOR_H
