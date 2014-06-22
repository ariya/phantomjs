/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <qcursor.h>
#include <qscreen.h>
#include <private/qcursor_p.h>
#include <qpa/qplatformcursor.h>
#include <private/qguiapplication_p.h>
#include <qbitmap.h>

QT_BEGIN_NAMESPACE

QT_USE_NAMESPACE

#ifndef QT_NO_CURSOR

/*****************************************************************************
  Internal QCursorData class
 *****************************************************************************/

QCursorData::QCursorData(Qt::CursorShape s)
    : ref(1), cshape(s), bm(0), bmm(0), hx(0), hy(0)
{
}

QCursorData::~QCursorData()
{
    delete bm;
    delete bmm;
}


/*****************************************************************************
  Global cursors
 *****************************************************************************/

extern QCursorData *qt_cursorTable[Qt::LastCursor + 1]; // qcursor.cpp

QCursorData *QCursorData::setBitmap(const QBitmap &bitmap, const QBitmap &mask, int hotX, int hotY)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (bitmap.depth() != 1 || mask.depth() != 1 || bitmap.size() != mask.size()) {
        qWarning("QCursor: Cannot create bitmap cursor; invalid bitmap(s)");
        QCursorData *c = qt_cursorTable[0];
        c->ref.ref();
        return c;
    }
    QCursorData *d = new QCursorData;
    d->bm  = new QBitmap(bitmap);
    d->bmm = new QBitmap(mask);
    d->cshape = Qt::BitmapCursor;
    d->hx = hotX >= 0 ? hotX : bitmap.width() / 2;
    d->hy = hotY >= 0 ? hotY : bitmap.height() / 2;

    return d;
}

void QCursorData::update()
{
}

#endif //QT_NO_CURSOR

QPoint QCursor::pos()
{
    return QCursor::pos(QGuiApplication::primaryScreen());
}

QPoint QCursor::pos(const QScreen *screen)
{
    if (screen)
        if (const QPlatformCursor *cursor = screen->handle()->cursor())
            return cursor->pos();
    return QGuiApplicationPrivate::lastCursorPosition.toPoint();
}

void QCursor::setPos(QScreen *screen, int x, int y)
{
    if (screen) {
        if (QPlatformCursor *cursor = screen->handle()->cursor()) {
            const QPoint pos = QPoint(x, y);
            // Need to check, since some X servers generate null mouse move
            // events, causing looping in applications which call setPos() on
            // every mouse move event.
            if (pos != cursor->pos())
                cursor->setPos(pos);
        }
    }
}

void QCursor::setPos(int x, int y)
{
    QCursor::setPos(QGuiApplication::primaryScreen(), x, y);
}

QT_END_NAMESPACE
