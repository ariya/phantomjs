/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QCURSOR_H
#define QCURSOR_H

#include <QtCore/qpoint.h>
#include <QtGui/qwindowdefs.h>

QT_BEGIN_NAMESPACE


class QVariant;
class QScreen;

/*
  ### The fake cursor has to go first with old qdoc.
*/
#ifdef QT_NO_CURSOR

class Q_GUI_EXPORT QCursor
{
public:
    static QPoint pos();
    static QPoint pos(const QScreen *screen);
    static void setPos(int x, int y);
    static void setPos(QScreen *screen, int x, int y);
    inline static void setPos(const QPoint &p) { setPos(p.x(), p.y()); }
private:
    QCursor();
};

#endif // QT_NO_CURSOR

#ifndef QT_NO_CURSOR

class QCursorData;
class QBitmap;
class QPixmap;


class Q_GUI_EXPORT QCursor
{
public:
    QCursor();
    QCursor(Qt::CursorShape shape);
    QCursor(const QBitmap &bitmap, const QBitmap &mask, int hotX=-1, int hotY=-1);
    QCursor(const QPixmap &pixmap, int hotX=-1, int hotY=-1);
    QCursor(const QCursor &cursor);
    ~QCursor();
    QCursor &operator=(const QCursor &cursor);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QCursor &operator=(QCursor &&other)
    { qSwap(d, other.d); return *this; }
#endif
    operator QVariant() const;

    Qt::CursorShape shape() const;
    void setShape(Qt::CursorShape newShape);

    const QBitmap *bitmap() const;
    const QBitmap *mask() const;
    QPixmap pixmap() const;
    QPoint hotSpot() const;

    static QPoint pos();
    static QPoint pos(const QScreen *screen);
    static void setPos(int x, int y);
    static void setPos(QScreen *screen, int x, int y);
    inline static void setPos(const QPoint &p) { setPos(p.x(), p.y()); }
    inline static void setPos(QScreen *screen, const QPoint &p) { setPos(screen, p.x(), p.y()); }

private:
    QCursorData *d;
};

/*****************************************************************************
  QCursor stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &outS, const QCursor &cursor);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &inS, QCursor &cursor);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QCursor &);
#endif

#endif // QT_NO_CURSOR

QT_END_NAMESPACE

#endif // QCURSOR_H
